# Semaphore-Lab-1
This Project Tackles the Producer-Consumer Problem in C++ using semaphores and mutexs and shared memory in a LINUX environment. 

# Description 
The producer file will generate items and input them into a table. The consumer will then grab the items from the table The table has a limit to how much it can carry at a time. When full the producer cannot add more to it. When its empty the Consumer must wait instead. Semaphones syncs these two processes and it uses mutal exclusion to keep it clean and make sure only one is in at a time. 

# Customize
You can change the ammount of created products by changing the value "int maxItems = 10;" or by adding a number like so " ./producer 20 & ./consumer & " with "20" being how many will be produced. 

# Example Of Output 

![My-intro-img](https://github.com/Zbush1/Semaphore-Lab-1/blob/main/Code_bupXIy0tBK.png)
