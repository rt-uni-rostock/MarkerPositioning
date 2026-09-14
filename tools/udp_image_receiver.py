#!/usr/bin/env python3
"""
UDP image receiver for MarkerPositioning (LiveImagePassthrough mode).

The receive loop runs in a dedicated thread so that imshow/disk I/O
never blocks incoming packets and causes chunk loss.

Usage examples:
  python udp_image_receiver.py                        # display only
  python udp_image_receiver.py --save-dir ./received  # save to disk, no display
  python udp_image_receiver.py --save-dir ./received --display  # save AND display
"""
import argparse
import os
import queue
import socket
import struct
import threading
import time
from dataclasses import dataclass, field
from typing import Dict, Optional, Tuple

import cv2
import numpy as np


MAGIC = 0x4D50494D  # "MPIM"
VERSION = 2
BYTE_ORDER_LITTLE_ENDIAN = 1
HEADER_STRUCT = struct.Struct("<I H B B Q q q i i i B B B B I I I H H")

DEPTH_TO_DTYPE = {
    0: np.uint8,    # CV_8U
    1: np.int8,     # CV_8S
    2: np.uint16,   # CV_16U
    3: np.int16,    # CV_16S
    4: np.int32,    # CV_32S
    5: np.float32,  # CV_32F
    6: np.float64,  # CV_64F
}


@dataclass
class FrameAssembly:
    camera_id: int
    frame_id: int
    capture_ts_ns: int
    receive_ts_ns: int
    width: int
    height: int
    cv_type: int
    channels: int
    elem_size_bytes: int
    depth_code: int
    total_bytes: int
    total_chunks: int
    chunk_stride_bytes: int
    created_at: float = field(default_factory=time.time)
    last_update_at: float = field(default_factory=time.time)
    data: bytearray = field(default_factory=bytearray)
    chunks_seen: bytearray = field(default_factory=bytearray)
    received_chunks: int = 0

    def __post_init__(self) -> None:
        self.data = bytearray(self.total_bytes)
        self.chunks_seen = bytearray(self.total_chunks)

    def add_chunk(self, chunk_index: int, payload: bytes) -> None:
        if self.chunks_seen[chunk_index]:
            self.last_update_at = time.time()
            return

        start = chunk_index * self.chunk_stride_bytes
        end = start + len(payload)
        if end > self.total_bytes:
            raise ValueError("Chunk exceeds expected frame size.")

        self.data[start:end] = payload
        self.chunks_seen[chunk_index] = 1
        self.received_chunks += 1
        self.last_update_at = time.time()

    def is_complete(self) -> bool:
        return self.received_chunks == self.total_chunks


@dataclass
class CompletedFrame:
    assembly: FrameAssembly
    image: np.ndarray
    latency_ms: float


def parse_header(packet: bytes):
    if len(packet) < HEADER_STRUCT.size:
        raise ValueError("Packet shorter than header size.")
    return HEADER_STRUCT.unpack_from(packet, 0)


def make_image(frame: FrameAssembly) -> np.ndarray:
    if frame.depth_code not in DEPTH_TO_DTYPE:
        raise ValueError(f"Unsupported depthCode: {frame.depth_code}")

    dtype = DEPTH_TO_DTYPE[frame.depth_code]
    if np.dtype(dtype).itemsize != frame.elem_size_bytes:
        raise ValueError(
            f"elemSizeBytes mismatch: header={frame.elem_size_bytes}, "
            f"expected={np.dtype(dtype).itemsize}"
        )

    arr = np.frombuffer(bytes(frame.data), dtype=dtype).copy()
    if frame.channels == 1:
        return arr.reshape((frame.height, frame.width))
    return arr.reshape((frame.height, frame.width, frame.channels))


def to_display_image(image: np.ndarray) -> np.ndarray:
    if image.dtype == np.uint8:
        return image
    return cv2.normalize(image, None, 0, 255, cv2.NORM_MINMAX).astype(np.uint8)


def receive_thread(
    sock: socket.socket,
    max_datagram_size: int,
    frame_timeout_sec: float,
    out_queue: "queue.Queue[CompletedFrame]",
    stop_event: threading.Event,
) -> None:
    """Dedicated receive loop — no blocking calls other than recvfrom."""
    assemblies: Dict[Tuple[int, int], FrameAssembly] = {}

    while not stop_event.is_set():
        now = time.time()
        stale_keys = [
            k for k, v in assemblies.items()
            if now - v.last_update_at > frame_timeout_sec
        ]
        for k in stale_keys:
            stale = assemblies.pop(k)
            print(
                f"[drop-timeout] camera={stale.camera_id} frame={stale.frame_id} "
                f"chunks={stale.received_chunks}/{stale.total_chunks}"
            )

        try:
            packet, _addr = sock.recvfrom(max_datagram_size)
        except socket.timeout:
            continue
        except OSError:
            break

        try:
            (
                magic, version, byte_order, camera_id,
                frame_id, capture_ts_ns, receive_ts_ns,
                width, height, cv_type,
                channels, elem_size_bytes, depth_code, _reserved0,
                total_image_bytes, total_chunks, chunk_index,
                chunk_bytes, chunk_stride_bytes,
            ) = parse_header(packet)
        except ValueError:
            continue

        if magic != MAGIC or version != VERSION or byte_order != BYTE_ORDER_LITTLE_ENDIAN:
            continue
        if total_chunks == 0 or chunk_stride_bytes == 0:
            continue
        if chunk_bytes == 0 or chunk_bytes > chunk_stride_bytes:
            continue
        if chunk_index >= total_chunks:
            continue

        payload = packet[HEADER_STRUCT.size:HEADER_STRUCT.size + chunk_bytes]
        if len(payload) != chunk_bytes:
            continue

        key = (camera_id, frame_id)
        if key not in assemblies:
            assemblies[key] = FrameAssembly(
                camera_id=camera_id,
                frame_id=frame_id,
                capture_ts_ns=capture_ts_ns,
                receive_ts_ns=receive_ts_ns,
                width=width,
                height=height,
                cv_type=cv_type,
                channels=channels,
                elem_size_bytes=elem_size_bytes,
                depth_code=depth_code,
                total_bytes=total_image_bytes,
                total_chunks=total_chunks,
                chunk_stride_bytes=chunk_stride_bytes,
            )

        frame_asm = assemblies[key]
        try:
            frame_asm.add_chunk(chunk_index, payload)
        except ValueError:
            assemblies.pop(key, None)
            continue

        if not frame_asm.is_complete():
            continue

        assemblies.pop(key)
        try:
            image = make_image(frame_asm)
        except ValueError as ex:
            print(f"[drop] camera={frame_asm.camera_id} frame={frame_asm.frame_id}: {ex}")
            continue

        latency_ms = (time.time_ns() - frame_asm.capture_ts_ns) / 1_000_000.0
        print(
            f"[frame] camera={frame_asm.camera_id} frame={frame_asm.frame_id} "
            f"{frame_asm.width}x{frame_asm.height} latency={latency_ms:.1f}ms"
        )

        try:
            out_queue.put_nowait(CompletedFrame(frame_asm, image, latency_ms))
        except queue.Full:
            print(f"[drop-queue-full] camera={frame_asm.camera_id} frame={frame_asm.frame_id}")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Reference UDP image receiver for MarkerPositioning."
    )
    parser.add_argument("--ip", default="0.0.0.0", help="Local bind IP")
    parser.add_argument("--port", type=int, default=5001, help="UDP port")
    parser.add_argument("--max-datagram-size", type=int, default=65535,
                        help="Max UDP datagram size to receive (bytes)")
    parser.add_argument("--frame-timeout-sec", type=float, default=8.0,
                        help="Drop incomplete frames after inactivity timeout")
    parser.add_argument("--save-dir", default=None,
                        help="Directory to save received frames as PNG files. "
                             "If not set, frames are only displayed.")
    parser.add_argument("--display", action="store_true",
                        help="Show frames in a window even when --save-dir is set.")
    args = parser.parse_args()

    show = args.save_dir is None or args.display

    if args.save_dir:
        os.makedirs(args.save_dir, exist_ok=True)
        print(f"Saving frames to: {args.save_dir}")

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 32 * 1024 * 1024)
    sock.bind((args.ip, args.port))
    sock.settimeout(0.2)

    frame_queue: "queue.Queue[CompletedFrame]" = queue.Queue(maxsize=8)
    stop_event = threading.Event()

    recv_thread = threading.Thread(
        target=receive_thread,
        args=(sock, args.max_datagram_size, args.frame_timeout_sec, frame_queue, stop_event),
        daemon=True,
    )
    recv_thread.start()

    print(f"Listening on {args.ip}:{args.port}  (press q to quit)")

    try:
        while True:
            try:
                completed: CompletedFrame = frame_queue.get(timeout=0.2)
            except queue.Empty:
                if show and cv2.waitKey(1) & 0xFF == ord("q"):
                    break
                continue

            frame_asm = completed.assembly
            image = completed.image

            if args.save_dir:
                filename = os.path.join(
                    args.save_dir,
                    f"cam{frame_asm.camera_id}_frame{frame_asm.frame_id:06d}.png",
                )
                cv2.imwrite(filename, image)

            if show:
                display = to_display_image(image)
                cv2.putText(
                    display,
                    f"frame={frame_asm.frame_id} latency={completed.latency_ms:.1f}ms",
                    (20, 30),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.8,
                    (0, 255, 0),
                    2,
                    cv2.LINE_AA,
                )
                cv2.imshow(f"camera-{frame_asm.camera_id}", display)
                if cv2.waitKey(1) & 0xFF == ord("q"):
                    break
    finally:
        stop_event.set()
        recv_thread.join(timeout=2.0)
        sock.close()
        if show:
            cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
