function udp_image_receiver_matlab(varargin)
% UDP_IMAGE_RECEIVER_MATLAB
% Receives MarkerPositioning UDP image chunks (protocol v2) and saves frames as PNG.
%
% Usage:
%   udp_image_receiver_matlab();
%   udp_image_receiver_matlab('BindIP','0.0.0.0','Port',5001,'SaveDir','received_matlab');
%
% No live display. Stop with Ctrl+C.
%
% Performance note: assemblies are tracked in a preallocated cell array
% indexed directly by cameraId (0..MaxCameraId), NOT a containers.Map.
% containers.Map has significant per-operation overhead in MATLAB (hashing +
% value copy on every read/write); at ~4500+ UDP chunks per 1080p frame this
% caused the receiver to fall behind the sender and accumulate latency.
% Direct array indexing avoids that overhead entirely.

p = inputParser;
addParameter(p, 'BindIP', '0.0.0.0', @(x)ischar(x) || isstring(x));
addParameter(p, 'Port', 5001, @(x)isnumeric(x) && isscalar(x));
addParameter(p, 'SaveDir', 'received_matlab', @(x)ischar(x) || isstring(x));
addParameter(p, 'FrameTimeoutSec', 8.0, @(x)isnumeric(x) && isscalar(x) && x > 0);
addParameter(p, 'PollTimeoutSec', 0.2, @(x)isnumeric(x) && isscalar(x) && x > 0);
addParameter(p, 'MaxCameraId', 31, @(x)isnumeric(x) && isscalar(x) && x >= 0);
parse(p, varargin{:});

bindIp = char(p.Results.BindIP);
port = double(p.Results.Port);
saveDir = char(p.Results.SaveDir);
frameTimeoutSec = double(p.Results.FrameTimeoutSec);
pollTimeoutSec = double(p.Results.PollTimeoutSec);
maxCameraId = double(p.Results.MaxCameraId);

if ~exist(saveDir, 'dir')
    mkdir(saveDir);
end

MAGIC = uint32(hex2dec('4D50494D')); % "MPIM"
VERSION = uint16(2);
HEADER_SIZE = 64; % bytes for packed C++ ImageChunkHeader

% One assembly slot per camera id (direct index, no hashing).
% Each slot is either [] (empty/no frame in progress) or an assembly struct.
assemblies = cell(1, maxCameraId + 1);

u = udpport("datagram","IPV4","LocalHost",bindIp,"LocalPort",port,"Timeout",pollTimeoutSec);
cleanupObj = onCleanup(@() clearUdp(u)); %#ok<NASGU>

fprintf('Listening on %s:%d\n', bindIp, port);
fprintf('Saving frames to: %s\n', saveDir);

lastTimeoutCheck = nowPosix();

while true
    nAvail = u.NumDatagramsAvailable;

    if nAvail == 0
        % Only check for stale/incomplete frames when idle, to keep the
        % hot receive path (below) as fast as possible while data is flowing.
        nowSec = nowPosix();
        if (nowSec - lastTimeoutCheck) > 0.5
            for camId = 0:maxCameraId
                a = assemblies{camId + 1};
                if ~isempty(a) && (nowSec - a.lastUpdateSec) > frameTimeoutSec
                    fprintf('[drop-timeout] camera=%d frame=%d chunks=%d/%d\n', ...
                        a.cameraId, a.frameId, a.receivedChunks, a.totalChunks);
                    assemblies{camId + 1} = [];
                end
            end
            lastTimeoutCheck = nowSec;
        end
        pause(0.001);
        continue;
    end

    d = read(u, nAvail, "uint8");

    % Pull the raw packet payloads out into a plain cell array ONCE per
    % batch. Repeatedly indexing the table/struct-array object itself
    % inside the loop (d.Data{di} / d(di).Data) re-invokes its overloaded
    % subsref on every single packet, which is a per-call table/struct
    % overhead on top of the actual data access - costly at thousands of
    % chunks per frame. A plain cell array has none of that overhead.
    if istable(d)
        dataCol = d.Data;
    else
        dataCol = {d.Data};
    end
    nRows = numel(dataCol);

    nowSec = nowPosix();

    for di = 1:nRows
        packet = dataCol{di};

        if numel(packet) < HEADER_SIZE
            continue;
        end

        [ok, h] = parseHeader(packet);
        if ~ok
            continue;
        end

        if h.magic ~= MAGIC || h.version ~= VERSION || h.byteOrder ~= uint8(1)
            continue;
        end
        if h.totalChunks == 0 || h.chunkStrideBytes == 0
            continue;
        end
        if h.chunkBytes == 0 || h.chunkBytes > h.chunkStrideBytes
            continue;
        end
        if h.chunkIndex >= h.totalChunks
            continue;
        end

        camId = double(h.cameraId);
        if camId < 0 || camId > maxCameraId
            continue; % camera id out of configured range, ignore
        end

        payloadStart = HEADER_SIZE + 1;
        payloadEnd = HEADER_SIZE + double(h.chunkBytes);
        if numel(packet) < payloadEnd
            continue;
        end
        payload = packet(payloadStart:payloadEnd);

        slot = camId + 1;
        a = assemblies{slot};
        % Immediately drop the cell array's reference to this assembly
        % before mutating it. As long as BOTH assemblies{slot} and the
        % local variable `a` reference the same underlying arrays,
        % MATLAB's copy-on-write semantics force a full deep copy of
        % a.data (up to ~6.2MB for a 1080p BGR frame) on the very next
        % in-place write below - for EVERY single chunk, not just once
        % per frame. That was costing ~1-1.7ms per packet (measured) and
        % was the actual dominant source of the growing per-frame latency.
        % Clearing the cell slot first makes `a` the sole owner, so the
        % write can happen in place with no copy.
        assemblies{slot} = [];

        frameId = double(h.frameId);

        % New frame for this camera, or a newer frameId superseding a
        % stale in-progress one (e.g. previous frame timed out silently).
        if isempty(a) || a.frameId ~= frameId
            a = struct();
            a.cameraId = camId;
            a.frameId = frameId;
            a.captureTsNs = double(h.captureTimestampNs);
            a.receiveTsNs = double(h.receiveTimestampNs);
            a.width = double(h.width);
            a.height = double(h.height);
            a.channels = double(h.channels);
            a.elemSizeBytes = double(h.elemSizeBytes);
            a.depthCode = double(h.depthCode);
            a.totalBytes = double(h.totalImageBytes);
            a.totalChunks = double(h.totalChunks);
            a.chunkStrideBytes = double(h.chunkStrideBytes);
            a.data = zeros(1, a.totalBytes, 'uint8');
            a.chunksSeen = false(1, a.totalChunks);
            a.receivedChunks = 0;
            % Diagnostic timestamps to break down where latency accumulates:
            % wall-clock time this receiver first saw ANY chunk of this frame.
            a.firstChunkWallSec = nowSec;
        end

        idx = double(h.chunkIndex) + 1;
        if ~a.chunksSeen(idx)
            startPos = double(h.chunkIndex) * a.chunkStrideBytes + 1;
            endPos = startPos + double(h.chunkBytes) - 1;
            if endPos > a.totalBytes
                assemblies{slot} = [];
                continue;
            end

            a.data(startPos:endPos) = payload;
            a.chunksSeen(idx) = true;
            a.receivedChunks = a.receivedChunks + 1;
        end

        a.lastUpdateSec = nowSec;

        if a.receivedChunks ~= a.totalChunks
            assemblies{slot} = a;
            continue;
        end

        % Frame complete: decode, save, and free the slot immediately.
        assemblies{slot} = [];
        wallReceiveDurationMs = (nowSec - a.firstChunkWallSec) * 1000;

        tDecode = tic;
        img = decodeImage(a);
        decodeMs = toc(tDecode) * 1000;
        if isempty(img)
            fprintf('[drop] camera=%d frame=%d: unsupported/invalid format\n', a.cameraId, a.frameId);
            continue;
        end

        fileName = sprintf('cam%d_frame%06d.png', a.cameraId, a.frameId);
        outPath = fullfile(saveDir, fileName);
        try
            % 'Compression','none' skips PNG deflate compression, which is
            % measurably slow (~100ms extra at 1080p) and is not needed
            % here since this is a local diagnostic/save tool, not a
            % storage-optimized archive.
            tSave = tic;
            imwrite(img, outPath, 'Compression', 'none');
            saveMs = toc(tSave) * 1000;

            nowNs = nowPosixNs();
            captureToReceiveMs = (a.receiveTsNs - a.captureTsNs) / 1e6;
            receiveToDoneMs = (nowNs - a.receiveTsNs) / 1e6;
            latencyMs = (nowNs - a.captureTsNs) / 1e6;
            fprintf(['[frame] camera=%d frame=%d %dx%d latency=%.1fms ' ...
                '(capture->cycle=%.1fms, cycle->done=%.1fms, sockRecv=%.1fms, decode=%.1fms, save=%.1fms) saved=%s\n'], ...
                a.cameraId, a.frameId, a.width, a.height, latencyMs, ...
                captureToReceiveMs, receiveToDoneMs, wallReceiveDurationMs, decodeMs, saveMs, outPath);
        catch ME
            fprintf('[drop-write] camera=%d frame=%d err=%s\n', a.cameraId, a.frameId, ME.message);
        end
    end
end

end

function [ok, h] = parseHeader(packet)
% IMPORTANT: read(u, n, "uint8") still hands back the packet bytes as
% MATLAB class "double" (values 0-255), not literally class "uint8" -
% despite the requested datatype. typecast() on a double array
% reinterprets raw 8-byte IEEE754 bit patterns instead of individual
% byte values, silently producing wrongly-shaped results (e.g. an
% 8-element uint32 array instead of a scalar). The uint8() cast below is
% therefore NOT redundant - it is required for typecast() to operate on
% actual single-byte values.
% Bounds are checked by the caller (numel(packet) < HEADER_SIZE), so the
% try/catch that used to wrap this is skipped too - try/catch has a
% non-trivial per-call cost in tight MATLAB loops.
h = struct();

b = uint8(packet);
p = 1;
h.magic = typecast(b(p:p+3), 'uint32'); p = p + 4;
h.version = typecast(b(p:p+1), 'uint16'); p = p + 2;
h.byteOrder = b(p); p = p + 1;
h.cameraId = b(p); p = p + 1;

h.frameId = typecast(b(p:p+7), 'uint64'); p = p + 8;
h.captureTimestampNs = typecast(b(p:p+7), 'int64'); p = p + 8;
h.receiveTimestampNs = typecast(b(p:p+7), 'int64'); p = p + 8;

h.width = typecast(b(p:p+3), 'int32'); p = p + 4;
h.height = typecast(b(p:p+3), 'int32'); p = p + 4;
h.cvType = typecast(b(p:p+3), 'int32'); p = p + 4;

h.channels = b(p); p = p + 1;
h.elemSizeBytes = b(p); p = p + 1;
h.depthCode = b(p); p = p + 1;
h.reserved0 = b(p); p = p + 1;

h.totalImageBytes = typecast(b(p:p+3), 'uint32'); p = p + 4;
h.totalChunks = typecast(b(p:p+3), 'uint32'); p = p + 4;
h.chunkIndex = typecast(b(p:p+3), 'uint32'); p = p + 4;
h.chunkBytes = typecast(b(p:p+1), 'uint16'); p = p + 2;
h.chunkStrideBytes = typecast(b(p:p+1), 'uint16');

ok = true;
end

function img = decodeImage(a)
img = [];

if a.width <= 0 || a.height <= 0 || a.channels <= 0
    return;
end
if a.elemSizeBytes ~= 1 || a.depthCode ~= 0
    return;
end

expectedElems = a.width * a.height * a.channels;
if numel(a.data) < expectedElems
    return;
end

raw = a.data(1:expectedElems);

if a.channels == 1
    img = reshape(raw, [a.width, a.height])';
else
    tmp = reshape(raw, [a.channels, a.width, a.height]);
    tmp = permute(tmp, [3, 2, 1]); % H x W x C
    if a.channels == 3
        img = tmp(:, :, [3, 2, 1]); % BGR -> RGB
    else
        img = tmp;
    end
end
end

function t = nowPosix()
t = posixtime(datetime('now','TimeZone','UTC'));
end

function tns = nowPosixNs()
tns = posixtime(datetime('now','TimeZone','UTC')) * 1e9;
end

function clearUdp(u)
try
    clear u
catch
end
end
