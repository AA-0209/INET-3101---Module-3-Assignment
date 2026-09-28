# INET-3101---Module-3-Assignment

# Problem & Solution Summary: Overview of your struct design and menu architecture.
- Each seat is stored in a small record with a seat number, if it's taken, and the person's first and last name. There are two lists of 24 seats for both outbound and inbound flights.
- The first menu asks which flight you want to board or if you want to quit. After you pick a flight the second menu lets you see empty seats and see passengers in alphabetical order,if you want to book a seat, cancel a booking or go back. When you book or cancel it will let you back out.

# Input Stream & Buffer Analysis: Technical breakdown of how your C g prevents standard input pollution when switching between character choices (getchar/scanf) and full string reads.
- The program reads each line you type at once so nothing is left over to mess up the next question that is asked. It also ignores any extra characters when a line is too long and asks again if the input is wrong.

# AI Test Harness Evaluation: Prompts used to generate test_input.txt, what bugs or buffer failures the batch test uncovered in your initial C code, and how you refactored the C code to fix them.
- I asked claude to make a test file full of bad inputs such as wrong menu choices or taken seats. It ended up having no bugs when the program ran because it read each line at once and check it before using it so nothing would cause any problems. 
