# Project-Booking-the-restaurant
This Project is all about using MQTT protocol to create web application. The Application is about restaurant reservation.
To see the report for each week, you can select by the braches of this repo.

# Overview 
![image](https://user-images.githubusercontent.com/87507926/141608816-c021ae5e-8774-4c82-a0cb-764c8e4f5c0b.png)

This picture show the overall tools that we used.
In details:

**Web Frontend**
 
 *UI*

- We use React.js for UI in web browser because it's fast and simple to use with node.js to create some mechanism in the client-side such as bind with mqtt to connect the backend or display some realtime feature.
 https://reactjs.org/
- We use CSS to create the responsive site and positioning and decorate all the UIs.
- We use Sweetalert2 for pop-up because it's pop-up out of the box and easy to implement.
 https://sweetalert2.github.io
- We use react-toastify for notification because it's easy to implement with react
 https://www.npmjs.com/package/react-toastify

(All of these are latest version)

 *Connecting*

- We use mqtt.js that use with node.js because the simplicity to use the mqtt protocol over the websocket.
 https://github.com/mqttjs/MQTT.js

**Backend**

*Environment*

- We develop and run on ubuntu 20.04 LTS because it's stable and has long term support.
- We use mosquitto version 2.0.13 as local mqtt broker because it's easy to implement the mqtt over websocket. So the backend side own the broker, database and queue system. 
https://mosquitto.org/

*System*

- We use C language for all the system and GUI development because ubuntu comes with gcc compiler and C is very fast so it serve our work efficiently.

*GUI*
 
 - We use gtk3+ to create the gui as POS program for employee to manage the queue because we've use it before and it suits with linux environment.
  https://docs.gtk.org/gtk3/

*Database*

 - We use mysql version.. as database because for our data , it suits with relational database then we have to use SQL language and mysql help us to manage all the database work and mysql has C library that is easy to use.
 - We also use mysql workbench version.. for helping us to develop the database side.
 

# Process

![image](https://user-images.githubusercontent.com/87507926/141610812-0a860c75-8395-45c6-90ac-a22716a12a82.png)


This picture show how all the process connect with each other.

So start from left to right (web frontend to backend).

 First, Client or user using website, entering the informations (Name,Number of guest,Tel) and the web frontend will automaticlly generate Client_id for authorize the user (using uuid to generate).
 Then, when user has sent the informations (with Client_id), they will go through mqtt over websocket and go to the broker with the topic call "request". In the other side Backend program will recieve the informations order by the broker then the program will connect to database to collect the data and to use data for queue system. After finish all the calculating and collecting data, the backend program will send the queue number and remaining queue for that specific user by sending back through topic call "respond/{Client_id}". Then the user will recieve respond datas through "respond/{Client_id}" topic and then save them on localstorage that have expired date information to delete day by day. We have to save them on localstorage because when user close the website after they submit it will not reset all the informations and status and we use localstorage for real time display 

