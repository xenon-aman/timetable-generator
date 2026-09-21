USE timetable_db;

CREATE TABLE assignments (
    id                INT UNSIGNED NOT NULL AUTO_INCREMENT,
    institution_id    INT UNSIGNED NOT NULL,
    class_id          INT UNSIGNED NOT NULL,
    subject_id        INT UNSIGNED NOT NULL,
    teacher_id        INT UNSIGNED NOT NULL,
    periods_per_week  TINYINT UNSIGNED NOT NULL,
    block_size        TINYINT UNSIGNED NOT NULL DEFAULT 1,
    PRIMARY KEY (id),
    UNIQUE KEY uq_assignments (class_id, subject_id, teacher_id),
    CONSTRAINT chk_assignments_counts
        CHECK (periods_per_week > 0 AND block_size > 0),
    CONSTRAINT fk_assignments_institution
        FOREIGN KEY (institution_id) REFERENCES institutions (id),
    CONSTRAINT fk_assignments_class
        FOREIGN KEY (class_id) REFERENCES classes (id),
    CONSTRAINT fk_assignments_subject
        FOREIGN KEY (subject_id) REFERENCES subjects (id),
    CONSTRAINT fk_assignments_teacher
        FOREIGN KEY (teacher_id) REFERENCES teachers (id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE timetables (
    id              INT UNSIGNED NOT NULL AUTO_INCREMENT,
    institution_id  INT UNSIGNED NOT NULL,
    score           INT NOT NULL DEFAULT 0,
    is_active       BOOLEAN NOT NULL DEFAULT FALSE,
    explanation     JSON NULL,
    created_at      TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    CONSTRAINT fk_timetables_institution
        FOREIGN KEY (institution_id) REFERENCES institutions (id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE entries (
    id              INT UNSIGNED NOT NULL AUTO_INCREMENT,
    institution_id  INT UNSIGNED NOT NULL,
    timetable_id    INT UNSIGNED NOT NULL,
    assignment_id   INT UNSIGNED NOT NULL,
    slot_id         INT UNSIGNED NOT NULL,
    room_id         INT UNSIGNED NOT NULL,
    PRIMARY KEY (id),
    UNIQUE KEY uq_entries_room_slot (timetable_id, slot_id, room_id),
    CONSTRAINT fk_entries_institution
        FOREIGN KEY (institution_id) REFERENCES institutions (id),
    CONSTRAINT fk_entries_timetable
        FOREIGN KEY (timetable_id) REFERENCES timetables (id) ON DELETE CASCADE,
    CONSTRAINT fk_entries_assignment
        FOREIGN KEY (assignment_id) REFERENCES assignments (id),
    CONSTRAINT fk_entries_slot
        FOREIGN KEY (slot_id) REFERENCES time_slots (id),
    CONSTRAINT fk_entries_room
        FOREIGN KEY (room_id) REFERENCES rooms (id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE rules (
    id              INT UNSIGNED NOT NULL AUTO_INCREMENT,
    institution_id  INT UNSIGNED NOT NULL,
    rule_key        VARCHAR(60) NOT NULL,
    kind            ENUM('must', 'nice') NOT NULL,
    weight          INT NOT NULL DEFAULT 1,
    enabled         BOOLEAN NOT NULL DEFAULT TRUE,
    PRIMARY KEY (id),
    UNIQUE KEY uq_rules_key (institution_id, rule_key),
    CONSTRAINT fk_rules_institution
        FOREIGN KEY (institution_id) REFERENCES institutions (id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;