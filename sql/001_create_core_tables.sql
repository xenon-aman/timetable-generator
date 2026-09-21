USE timetable_db;

CREATE TABLE institutions (
    id          INT UNSIGNED NOT NULL AUTO_INCREMENT,
    name        VARCHAR(150) NOT NULL,
    type        ENUM('school', 'college') NOT NULL,
    created_at  TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE classes (
    id              INT UNSIGNED NOT NULL AUTO_INCREMENT,
    institution_id  INT UNSIGNED NOT NULL,
    name            VARCHAR(100) NOT NULL,
    strength        INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (id),
    UNIQUE KEY uq_classes_name (institution_id, name),
    CONSTRAINT fk_classes_institution
        FOREIGN KEY (institution_id) REFERENCES institutions (id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE subjects (
    id              INT UNSIGNED NOT NULL AUTO_INCREMENT,
    institution_id  INT UNSIGNED NOT NULL,
    name            VARCHAR(100) NOT NULL,
    is_lab          BOOLEAN NOT NULL DEFAULT FALSE,
    PRIMARY KEY (id),
    UNIQUE KEY uq_subjects_name (institution_id, name),
    CONSTRAINT fk_subjects_institution
        FOREIGN KEY (institution_id) REFERENCES institutions (id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE rooms (
    id              INT UNSIGNED NOT NULL AUTO_INCREMENT,
    institution_id  INT UNSIGNED NOT NULL,
    name            VARCHAR(100) NOT NULL,
    room_type       ENUM('classroom', 'lab') NOT NULL DEFAULT 'classroom',
    capacity        INT UNSIGNED NOT NULL,
    PRIMARY KEY (id),
    UNIQUE KEY uq_rooms_name (institution_id, name),
    CONSTRAINT fk_rooms_institution
        FOREIGN KEY (institution_id) REFERENCES institutions (id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE time_slots (
    id              INT UNSIGNED NOT NULL AUTO_INCREMENT,
    institution_id  INT UNSIGNED NOT NULL,
    day_of_week     TINYINT UNSIGNED NOT NULL,
    period_no       TINYINT UNSIGNED NOT NULL,
    start_time      TIME NOT NULL,
    end_time        TIME NOT NULL,
    is_break        BOOLEAN NOT NULL DEFAULT FALSE,
    PRIMARY KEY (id),
    UNIQUE KEY uq_slots_position (institution_id, day_of_week, period_no),
    CONSTRAINT chk_slots_day CHECK (day_of_week BETWEEN 1 AND 7),
    CONSTRAINT fk_slots_institution
        FOREIGN KEY (institution_id) REFERENCES institutions (id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;