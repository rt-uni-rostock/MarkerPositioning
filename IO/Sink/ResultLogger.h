#pragma once
#include "LogEvent.h"
#include "02_Detection/Pose.h"
#include <sqlite3.h>
#include <string>

// SQLite-based result logger.
// Uses: 
//  - WAL mode (write-ahead logging) for better performance in concurrent read/write scenarios.
//  - Prepared statements for efficient repeated inserts.
//  - Batch transactions to group multiple inserts into one transaction, improving performance.

class ResultLogger {
public:
	// constructor and destructor
	ResultLogger(const std::string& dbPath);
	~ResultLogger();

	// sqlite transaction management
	void beginTransaction();
	void commitTransaction();
	void rollbackTransaction();

	// callable method to log a single event, called by logging worker thread in Sink
	void logEvent(const LogEvent& event);

	bool isHealthy() const;

private:
	// helper methods for sqlite setup
	// creates results table if it doesnt exist
	void createTableIfNeeded();
	// prepares the insert statement for logging events, called in constructor
	void prepareStatements();
	// logs a single row for a Result event: either for a specific marker pose,
	// or for a "no marker" case (pose == nullptr, markerId typically -1)
	void logResultRow(const std::string& eventTimestamp, const PipelineResult& r, int markerId, const Pose* pose);

	// sqlite database connection and prepared statement handle
	sqlite3* db_ = nullptr;
	sqlite3_stmt* insertStmt_ = nullptr;

	bool transactionActive_ = false;
	bool healthy_ = true;
};