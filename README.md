This project is a sample of implementation of Loebner Prize Protocol 2 ( https://github.com/jhudsy/LoebnerPrizeProtocol ) in C++.

# Getting Started
Download the project and compile it with a C++ compilator on Windows (SEVEN minimum). This project have ben tested with Visual Studio Express, but there is nothing specific to a compilator.

## Generalities
The WebSocket module implements a Web Socket IO to communicate with the Judge program of Loebner Protocol via internet. The LoebnerInterface module manages the Loebner protocol-specific part and exchanges with the Web Socket. The LoebnerBot module manages the HMI dialog with the user on one hand, and the interface with the chatbot on the other hand:

JUDGE <==> internet <==> WebSocket <==> LoebnerInterface <==> LoebnerBot <==> Your bot


The bot showed in example here only repeats the questions of the judge. I'm not sure it will pass the Turing test. Then you will need to adapt it to your chatbot.

## Adaptation to your chatbot
Step 1: Create a nice icon and put it in the place of the existing icon.
Step 2: Customize the title of the dialog box
Step 3: Program the launch of your chatbot when a round is going to start
Step 4: Program the stop of your chatbot when a round is over
Step 5: Program the sending of judge's questions to your chatbot, and your chatbot's response to the judge.

Steps are noted "TODO" in program, search it to find the parts you have to change.
