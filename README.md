📝 GITHUB DESCRIPTION (SHORT)

A complete Student Management System in C with login authentication, role-based access (Admin, Teacher, Student, Guest), file handling, CGPA calculation, grace marks, attendance tracking, backups, and subject difficulty analytics.

🏷️ GITHUB TOPIC TAGS

Add these tags to your repo:

c
student-management-system
mini-project
college-project
file-handling
cgpa-calculator
console-application
education-system

📘 README.md (FULL PROFESSIONAL VERSION)
🎓 Student Management System (C Project)

A fully functional Student Management System developed in C, featuring login authentication, secure password handling, role-based dashboards (Admin, Teacher, Student, Guest), academic performance analytics, CGPA calculation, hard subject detection, and automatic backup generation.

🚀 Features
🔐 Login System

Password masking

Secure file-based authentication

Stores & updates last login time

Supports 4 roles:

Admin (full control)

Teacher

Student

Guest

🎭 Role-Based Menus
⭐ Admin Features

Add new students

Display all students

Search students by roll

Hard subject difficulty detection

Create new users

Reset password

Create timestamped backup

⭐ Teacher Features

View list of students

View student details

Analyze performance & backlogs

See hard subjects

⭐ Student Features

View own academic details

CGPA, marks, attendance, backlogs

Graduation eligibility

⭐ Guest Features

Limited student display

📂 Files Used
1️⃣ credentials.txt

Stores login users in the format:

username password role lastLogin


Example:

admin admin123 admin FIRST
teacher teacher123 teacher FIRST
student student123 student FIRST
guest guest123 guest FIRST

2️⃣ student.txt

Stores student records in this format:

roll name branch sem subjects attendance
subjectName1 marks1
subjectName2 marks2
...
- -


Example:

101 Rahul CSE 3 3 92
Maths 78
DS 35
CO 88
- -

🧮 Academic Calculations

CGPA calculation

Grace marks (marks 35–39 → +5)

Backlog count

Attendance validation

Student-wise and subject-wise performance analysis

Hard subject detection (based on fail percentage)

💾 Backup System

Admin can generate full backups with timestamps:

backup_student_202512061412.txt

🔧 How to Compile & Run
Using GCC:
gcc project.c -o sms
./sms


Make sure these files are in same folder:

project.c
credentials.txt
student.txt

🔑 Default Login Credentials
Role	Username	Password
Admin	admin	admin123
Teacher	teacher	teacher123
Student	student	student123
Guest	guest	guest123
