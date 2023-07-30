# SUMMARY

This is a project with algorithm RANSAC (Random Sample Consensus) is a robust statistical method widely used in computer vision and other fields for solving problems related to data fitting, model estimation, and inlier detection. This project uses threads to reduce execution time by splitting the iterations for each available computer core.

## COMPILATION

    ```bash
        make
    ```
    
    If you have a problem with library pthread, change the instruction of compilation in the Makefile file, add the flag:

    ```bash
        gcc -Wall -Wextra -o ransac main.c -lpthread
    ```

## EXECUTION

    ```bash
        ./ransac -N <number_of_iterations>
    ```
    
