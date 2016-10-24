CREATE USER 'kueued'@'localhost' IDENTIFIED BY '<password>';
GRANT USAGE ON *.* TO 'kueued'@'localhost';
GRANT ALL PRIVILEGES ON `kueued`.* TO 'kueued'@'localhost' WITH GRANT OPTION;

