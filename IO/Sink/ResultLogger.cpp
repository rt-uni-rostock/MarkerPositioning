#include "ResultLogger.h"
#include <stdexcept>

ResultLogger::ResultLogger(const std::string& dbPath) {
	if (sqlite3_open(dbPath.c_str(), &db_) != SQLITE_OK) {
		throw std::runtime_error("Failed to open SQLite database");
	}

	// Enable WAL mode for better concurrent performance
	sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);

	createTableIfNeeded();
	prepareStatements();
}

ResultLogger::~ResultLogger() {
	if (insertStmt_) {
		sqlite3_finalize(insertStmt_);
	}
	if (db_) {
		sqlite3_close(db_);
	}
}

void ResultLogger::createTableIfNeeded() {
	const char* sql =
		"CREATE TABLE IF NOT EXISTS results ("
		"id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"event_timestamp INTEGER,"
		"event_type INTEGER,"
		"image_timestamp INTEGER,"
		"marker_id INTEGER,"
		"camera_id INTEGER,"
		"marker_type INTEGER,"
		"error_code INTEGER,"
		"pos_x REAL,"
		"pos_y REAL,"
		"pos_z REAL,"
		"rot_x REAL,"
		"rot_y REAL,"
		"rot_z REAL,"
		"error_message TEXT,"
		"drop_count INTEGER"
		");";

	sqlite3_exec(db_, sql, nullptr, nullptr, nullptr);
}

void ResultLogger::prepareStatements() {
	const char* sql =
		"INSERT INTO results ("
		"event_timestamp, event_type, image_timestamp, marker_id, camera_id, marker_type,"
		"error_code, pos_x, pos_y, pos_z, rot_x, rot_y, rot_z, error_message, drop_count"
		") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

	sqlite3_prepare_v2(db_, sql, -1, &insertStmt_, nullptr);
}

void ResultLogger::beginTransaction() {
	if (!healthy_)
		return;

	if (transactionActive_)
		return; // already active

	int rc = sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
	if (rc != SQLITE_OK)
	{
		healthy_ = false;
		throw std::runtime_error("SQLite BEGIN failed");
	}

	transactionActive_ = true;
}

void ResultLogger::commitTransaction() {
	if (!transactionActive_)
		return;

	int rc = sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, nullptr);
	if (rc != SQLITE_OK)
	{
		transactionActive_ = false;
		healthy_ = false;
		throw std::runtime_error("SQLite COMMIT failed");
	}

	transactionActive_ = false;
}

void ResultLogger::rollbackTransaction() {
	if (!transactionActive_)
		return;

	int rc = sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);

	if (rc != SQLITE_OK)
	{
		// If rollback fails, DB may be corrupted.
		healthy_ = false;
	}

	transactionActive_ = false;
}

// logs one row per detected marker for Result events; a single "no marker" row (markerId = -1)
// if no markers were detected; a single row for Drop and SystemError events.
void ResultLogger::logEvent(const LogEvent& event) {
	if (event.type == LogEventType::Result) {
		const PipelineResult& r = event.result;

		if (r.detectedMarkers.empty()) {
			logResultRow(event.eventTimestamp, r, /*markerId*/ -1, /*pose*/ nullptr);
		}
		else {
			for (const auto& pose : r.detectedMarkers) {
				logResultRow(event.eventTimestamp, r, pose.tagId, &pose);
			}
		}
	}
	else if (event.type == LogEventType::Drop) {
		sqlite3_reset(insertStmt_);
		sqlite3_bind_text(insertStmt_, 1, event.eventTimestamp.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_int(insertStmt_, 2, static_cast<int>(event.type));

		sqlite3_bind_text(insertStmt_, 3, event.dropped.imageTimestamp.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_null(insertStmt_, 4); // marker_id not relevant for Drop (whole camera result dropped)
		sqlite3_bind_int(insertStmt_, 5, event.dropped.cameraId);
		sqlite3_bind_int(insertStmt_, 6, event.dropped.markerType);
		sqlite3_bind_null(insertStmt_, 7); // error_code not relevant for Drop
		sqlite3_bind_null(insertStmt_, 8); // pos_x not relevant for Drop
		sqlite3_bind_null(insertStmt_, 9); // pos_y not relevant for Drop
		sqlite3_bind_null(insertStmt_, 10); // pos_z not relevant for Drop
		sqlite3_bind_null(insertStmt_, 11); // rot_x not relevant for Drop
		sqlite3_bind_null(insertStmt_, 12); // rot_y not relevant for Drop
		sqlite3_bind_null(insertStmt_, 13); // rot_z not relevant for Drop
		sqlite3_bind_text(insertStmt_, 14, "UDP result dropped (backlog)", -1, SQLITE_TRANSIENT);
		sqlite3_bind_int64(insertStmt_, 15, event.dropCount);

		sqlite3_step(insertStmt_);
	}
	else if (event.type == LogEventType::SystemError) {
		sqlite3_reset(insertStmt_);
		sqlite3_bind_text(insertStmt_, 1, event.eventTimestamp.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_int(insertStmt_, 2, static_cast<int>(event.type));

		sqlite3_bind_null(insertStmt_, 3); // image_timestamp not relevant for SystemError
		sqlite3_bind_null(insertStmt_, 4); // marker_id not relevant for SystemError
		sqlite3_bind_null(insertStmt_, 5); // camera_id not relevant for SystemError
		sqlite3_bind_null(insertStmt_, 6); // marker_type not relevant for SystemError
		sqlite3_bind_null(insertStmt_, 7); // error_code not relevant for SystemError
		sqlite3_bind_null(insertStmt_, 8); // pos_x not relevant for SystemError
		sqlite3_bind_null(insertStmt_, 9); // pos_y not relevant for SystemError
		sqlite3_bind_null(insertStmt_, 10); // pos_z not relevant for SystemError
		sqlite3_bind_null(insertStmt_, 11); // rot_x not relevant for SystemError
		sqlite3_bind_null(insertStmt_, 12); // rot_y not relevant for SystemError
		sqlite3_bind_null(insertStmt_, 13); // rot_z not relevant for SystemError
		sqlite3_bind_text(insertStmt_, 14, event.systemMessage.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_null(insertStmt_, 15); // drop_count not relevant for SystemError

		sqlite3_step(insertStmt_);
	}
}

void ResultLogger::logResultRow(const std::string& eventTimestamp, const PipelineResult& r, int markerId, const Pose* pose)
{
	sqlite3_reset(insertStmt_);

	sqlite3_bind_text(insertStmt_, 1, eventTimestamp.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(insertStmt_, 2, static_cast<int>(LogEventType::Result));

	sqlite3_bind_text(insertStmt_, 3, r.imageTimestamp.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(insertStmt_, 4, markerId);
	sqlite3_bind_int(insertStmt_, 5, r.cameraId);
	sqlite3_bind_int(insertStmt_, 6, r.markerType);
	sqlite3_bind_int(insertStmt_, 7, r.errorCode);

	if (pose) {
		sqlite3_bind_double(insertStmt_, 8, pose->x);
		sqlite3_bind_double(insertStmt_, 9, pose->y);
		sqlite3_bind_double(insertStmt_, 10, pose->z);
		sqlite3_bind_double(insertStmt_, 11, pose->roll);
		sqlite3_bind_double(insertStmt_, 12, pose->pitch);
		sqlite3_bind_double(insertStmt_, 13, pose->yaw);
	}
	else {
		sqlite3_bind_null(insertStmt_, 8);
		sqlite3_bind_null(insertStmt_, 9);
		sqlite3_bind_null(insertStmt_, 10);
		sqlite3_bind_null(insertStmt_, 11);
		sqlite3_bind_null(insertStmt_, 12);
		sqlite3_bind_null(insertStmt_, 13);
	}

	sqlite3_bind_text(insertStmt_, 14, r.errorMessage.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_null(insertStmt_, 15); // drop_count not relevant for Result

	sqlite3_step(insertStmt_);
}

bool ResultLogger::isHealthy() const {
	return healthy_;
}