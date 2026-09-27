#pragma once
#include <string>
#include <vector>
#include "db.hpp"

// One "thing that needs to be scheduled": a class needs a subject,
// taught by a teacher, for some number of periods per week.
struct Assignment {
    int id;
    int classId;
    int subjectId;
    int teacherId;
    int periodsPerWeek;
    int blockSize;   // 1 = single period, 2 = double period (e.g. labs)
    bool isLab;
};

// One slot on the timetable grid: a specific day and period number.
struct TimeSlot {
    int id;
    int dayOfWeek;   // 1 = Monday ... 5 = Friday
    int periodNo;
    bool isBreak;
};

class Solver {
public:
    Solver(Database& db, int institutionId) : db(db), institutionId(institutionId) {}

    // Loads all assignments and time slots for this institution from the database.
    void loadData() {
        auto assignmentRows = db.queryPrepared(
            "SELECT a.id, a.class_id, a.subject_id, a.teacher_id, a.periods_per_week, a.block_size, s.is_lab "
            "FROM assignments a JOIN subjects s ON s.id = a.subject_id "
            "WHERE a.institution_id = ?",
            {std::to_string(institutionId)}
        );

        for (auto& row : assignmentRows) {
            Assignment a;
            a.id = std::stoi(row["id"]);
            a.classId = std::stoi(row["class_id"]);
            a.subjectId = std::stoi(row["subject_id"]);
            a.teacherId = std::stoi(row["teacher_id"]);
            a.periodsPerWeek = std::stoi(row["periods_per_week"]);
            a.blockSize = std::stoi(row["block_size"]);
            a.isLab = (row["is_lab"] == "1");
            assignments.push_back(a);
        }

        auto slotRows = db.queryPrepared(
            "SELECT id, day_of_week, period_no, is_break FROM time_slots "
            "WHERE institution_id = ? AND is_break = 0 ORDER BY day_of_week, period_no",
            {std::to_string(institutionId)}
        );

        for (auto& row : slotRows) {
            TimeSlot s;
            s.id = std::stoi(row["id"]);
            s.dayOfWeek = std::stoi(row["day_of_week"]);
            s.periodNo = std::stoi(row["period_no"]);
            s.isBreak = (row["is_break"] == "1");
            timeSlots.push_back(s);
        }
    }

    // Temporary, for testing Stage 1: prints what was loaded.
    void printSummary() {
        std::cout << "Loaded " << assignments.size() << " assignments and "
                   << timeSlots.size() << " non-break time slots.\n";
        for (auto& a : assignments) {
            std::cout << "  Assignment " << a.id << ": class " << a.classId
                       << ", subject " << a.subjectId << ", teacher " << a.teacherId
                       << ", " << a.periodsPerWeek << "x/week, block " << a.blockSize
                       << (a.isLab ? " (lab)" : "") << "\n";
        }
    }

private:
    Database& db;
    int institutionId;
    std::vector<Assignment> assignments;
    std::vector<TimeSlot> timeSlots;
};