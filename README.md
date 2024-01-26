# Lab6 submission

See PDF in the submission folder for the write-up.

Run the make command in terminal to create executables for CovertChannel.c and ExceptionSuppression.c on your local machine. 
To run them, use the commands: ./CovertChannel or ./ExceptionSuppression depending on the file you'd like to run.

To test the Meltdown.c file, copy it over to the lab server and change the TARGET_ADDRESS variable to your target address. Then build with gcc Meltdown.c -o Meltdown and use the command 'taskset -c 0 ./Meltdown' to run it. The 'taskset -c 0' makes it target core 0. 
