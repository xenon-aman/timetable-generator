USE timetable_db;

INSERT INTO institutions (name, type) VALUES ('Demo Public School', 'school');
SET @inst = LAST_INSERT_ID();

INSERT INTO classes (institution_id, name, strength) VALUES
    (@inst, 'Class 8A', 30),
    (@inst, 'Class 8B', 28);

INSERT INTO subjects (institution_id, name, is_lab) VALUES
    (@inst, 'Mathematics', FALSE),
    (@inst, 'Science', FALSE),
    (@inst, 'English', FALSE),
    (@inst, 'Computer Lab', TRUE);

INSERT INTO rooms (institution_id, name, room_type, capacity) VALUES
    (@inst, 'Room 101', 'classroom', 35),
    (@inst, 'Room 102', 'classroom', 35),
    (@inst, 'Computer Lab 1', 'lab', 32);

INSERT INTO teachers (institution_id, name) VALUES
    (@inst, 'Mrs. Sharma'),
    (@inst, 'Mr. Verma'),
    (@inst, 'Ms. Gupta'),
    (@inst, 'Mr. Khan');

-- Monday to Friday, 6 periods of 45 minutes starting at 8:00
INSERT INTO time_slots (institution_id, day_of_week, period_no, start_time, end_time, is_break)
SELECT @inst, d.n, p.n,
       ADDTIME('08:00:00', SEC_TO_TIME((p.n - 1) * 2700)),
       ADDTIME('08:45:00', SEC_TO_TIME((p.n - 1) * 2700)),
       FALSE
FROM (SELECT 1 AS n UNION SELECT 2 UNION SELECT 3 UNION SELECT 4 UNION SELECT 5) d
CROSS JOIN (SELECT 1 AS n UNION SELECT 2 UNION SELECT 3 UNION SELECT 4
            UNION SELECT 5 UNION SELECT 6) p;

-- Each class gets every subject; the lab is one block of 2 periods
INSERT INTO assignments (institution_id, class_id, subject_id, teacher_id, periods_per_week, block_size)
SELECT @inst, c.id, s.id, t.id, x.ppw, x.blk
FROM (SELECT 'Mathematics' AS subj, 'Mrs. Sharma' AS tname, 5 AS ppw, 1 AS blk
      UNION ALL SELECT 'Science', 'Mr. Verma', 4, 1
      UNION ALL SELECT 'English', 'Ms. Gupta', 4, 1
      UNION ALL SELECT 'Computer Lab', 'Mr. Khan', 2, 2) x
JOIN subjects s ON s.name = x.subj AND s.institution_id = @inst
JOIN teachers t ON t.name = x.tname AND t.institution_id = @inst
CROSS JOIN classes c
WHERE c.institution_id = @inst;
   SELECT COUNT(*) FROM time_slots;