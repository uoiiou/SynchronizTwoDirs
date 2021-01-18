# SynchronizTwoDirs

Synchronizing two directories, for example Dir1 and Dir2. The user specifies the names Dir1 and Dir2. As a result of the program files present in Dir1 but not in Dir2 should be copied in Dir2 along with the permissions. Copy routines must be run in a separate process for each file to be copied. Each process displays its pid, full path to the copied file and number copied bytes. The number of concurrent processes should not be exceed N (entered by the user).
