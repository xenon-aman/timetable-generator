USE timetable_db;

CREATE TABLE users (
    id              INT UNSIGNED NOT NULL AUTO_INCREMENT,
    institution_id  INT UNSIGNED NOT NULL,
    email           VARCHAR(190) NOT NULL,
    password_hash   VARCHAR(255) NOT NULL,
    role            ENUM('admin', 'teacher', 'student') NOT NULL,
    class_id        INT UNSIGNED NULL,
    created_at      TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uq_users_email (institution_id, email),
    CONSTRAINT fk_users_institution
        FOREIGN KEY (institution_id) REFERENCES institutions (id),
    CONSTRAINT fk_users_class
        FOREIGN KEY (class_id) REFERENCES classes (id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE teachers (
    id               INT UNSIGNED NOT NULL AUTO_INCREMENT,
    institution_id   INT UNSIGNED NOT NULL,
    user_id          INT UNSIGNED NULL,
    name             VARCHAR(120) NOT NULL,
    max_periods_day  TINYINT UNSIGNED NOT NULL DEFAULT 6,
    PRIMARY KEY (id),
    UNIQUE KEY uq_teachers_user (user_id),
    CONSTRAINT fk_teachers_institution
        FOREIGN KEY (institution_id) REFERENCES institutions (id),
    CONSTRAINT fk_teachers_user
        FOREIGN KEY (user_id) REFERENCES users (id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;