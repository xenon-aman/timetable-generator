USE timetable_db;

SET @inst = (SELECT id FROM institutions WHERE name = 'Demo Public School' LIMIT 1);
SET @class8a = (SELECT id FROM classes WHERE institution_id = @inst AND name = 'Class 8A' LIMIT 1);

INSERT INTO users (institution_id, email, password_hash, role, class_id) VALUES
(@inst, 'admin@demoschool.test',
 '$argon2id$v=19$m=65536,t=2,p=1$87Y9BEOX62Rg/MyIoZxxkg$gZcHcvz7/xWIGIk4AIQ9qwPhp7KN8ZZBzEA+3KG8F40',
 'admin', NULL),
(@inst, 'teacher@demoschool.test',
 '$argon2id$v=19$m=65536,t=2,p=1$8/nVMkNqe9/IH2+u0SBIlw$p9o2L3g79iLLFzYgKdfYS5imWLUG4ooSITpZRq/FUHw',
 'teacher', NULL),
(@inst, 'student@demoschool.test',
 '$argon2id$v=19$m=65536,t=2,p=1$cBtCIG4tlORY2FMRZKTYag$L+djgJQOru9CxRU42IVdMBi8ERH/LIw+YSQk38gYfzA',
 'student', @class8a);

-- Link the teacher account to an existing teacher row (Mrs. Sharma), so
-- "who is this teacher" and "which login is theirs" point to the same person
UPDATE teachers
SET user_id = (SELECT id FROM users WHERE email = 'teacher@demoschool.test')
WHERE institution_id = @inst AND name = 'Mrs. Sharma';