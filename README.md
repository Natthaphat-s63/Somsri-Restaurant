# Project Somsri restaurant
This Project is all about using MQTT protocol to create web application. The Application is about restaurant reservation.
To see the report for each week, you can select by the braches of this repo.

*****2 folders above are the final product of this project.**


For more detail you can check this out :
https://youtu.be/WvIgsM9eyjk

# Overview 
![image](https://user-images.githubusercontent.com/87507926/141608816-c021ae5e-8774-4c82-a0cb-764c8e4f5c0b.png)

This picture show the overall tools that we used.

for more detail:

**Web Frontend**
 
 *UI*

- We use React.js version 17.0.2 for UI in web browser because it's fast and simple to use with node.js to create some mechanism in the client-side such as bind with mqtt to connect the backend or display some realtime feature.
 https://reactjs.org/
- We use CSS to create the responsive site (by using Media query method) and positioning and decorate all the UIs.
- We use Sweetalert2 version 11.1.9 for pop-up because it has pop-ups out of the box and easy to implement.
 https://sweetalert2.github.io
- We use react-toastify version 8.0.3 for notification because it's easy to implement with react
 https://www.npmjs.com/package/react-toastify
 
 - We use UUID version 8.3.2 for generate uniqe Client_id.
 https://www.npmjs.com/package/uuid

 *Connecting*

- We use Node.js version 15.14.0 for implementing the mqtt.js package and implement all the mechanism of the webpage along with React.js
- We use MQTT.js version 4.2.8 that use with node.js because the simplicity to use the mqtt protocol over the websocket.
 https://github.com/mqttjs/MQTT.js

**Backend**

*Environment*

- We develop and run on ubuntu 20.04 LTS because it's stable and has long term support.
- We use mosquitto version 2.0.13 as local mqtt broker because it's easy to implement the mqtt over websocket and we have set it to automatically open the broker on startup of the OS. So the backend side own the broker, database and queue system. 
https://mosquitto.org/
- We use Visual Studio code IDE version 1.62.2 for develop both backend and frontend sides because it easy to use and it has many tools and feature to help develop this project faster than using other IDE.
- We use GCC Compiler version 9.3.0 that comes with ubuntu 20.04 LTS.

*Backend development tools*

- We use C language version C17 for all the system and GUI development because ubuntu comes with gcc compiler and C is very fast so it serve our work efficiently.
- We use paho mqtt library version 1.3.9 for implementing mqtt in C language because it's more flexible than mosquitto.h library.  
https://www.eclipse.org/paho/
- We use mysql.h for communicate with database from C program.
https://zetcode.com/db/mysqlc/
- We use json-c because we use JSON format for sending/recieve the datas through mqtt broker from user to restaurant. 
https://github.com/json-c/json-c
- We use pthread.h for multithread to perform parallel task along with main task.


*GUI*
 
 - We use gtk3 to create the gui as POS program for employee to manage the queue because we've use it before and it suits with linux environment. We don't use latest version of gtk because gtk3 is more stable and have used widly so there are many use case out there.
  https://docs.gtk.org/gtk3/

*Database*

 - We use mysql version 8.0.27 as database because for our data , it suits with relational database then we have to use SQL language and mysql help us to manage all the database work and mysql has C library that is easy to use.
 - We also use mysql workbench version 8.026 for helping us to develop the database side.
 

# Process

![image](https://user-images.githubusercontent.com/87507926/141610812-0a860c75-8395-45c6-90ac-a22716a12a82.png)


This picture show how all the process connect with each other.

So start from left to right (web frontend to backend).

First, Client or user using website, entering the informations (Name,Number of guest,Tel) and the web frontend will automaticlly generate Client_id for authorize the user (using uuid to generate). Then, when user has sent the informations (with Client_id), they will go through mqtt over websocket and go to the broker with the topic call "request". In the other side Backend program will recieve the informations order by the broker then the program will connect to database to collect the data and to use data for queue system. After finish all the calculating and collecting data, the backend program will send the queue number and remaining queue for that specific user by sending back through topic call "respond/{Client_id}". Then the user will recieve respond datas through "respond/{Client_id}" topic and then save them on localstorage that have expired date information to delete day by day. We have to save them on localstorage because when user close the website after they submit it will not reset all the informations and status.
And then the backend program will send the queue number and remaining queue everytime the queue list has changed so the web frontend can save and display updated remaining queue data.

That's all for the main task but there are 2 parallel tasks that are working along but use the same kind of connection like in the picture.

- First task is realtime recent remaining queue that show before user submit the informations so the user can check that if they booked how many queues that user have to waiting. the recent remaining queue will process by backend program using multithread to work along other tasks. the recent remaining queue will sent by backend through "recent_remianing" topic.

- Second task is Cancellation from both user and from restaurant and Confirmation from restaurant. User can cancel their queue then sending the cancel request in the "request" topic then the backend program will get the request and cancel the queue and update the remaining queue. After user cancel the web browser will automaticly wipe out all the datas and refresh the webpage. For the restaurant side the cancellation and confirmation will manage by the employee of the restaurant or who that has this duty and send the responf to specific user that they are has been cancel or confirm the queue by restaurant then the web page will wipe out all the datas and refresh the webpage.

# Database Design

![image](https://user-images.githubusercontent.com/87507926/142173485-5bed067f-da77-4d38-9b8c-31cab48dbdfa.png)

We decided to use relational database with 9 columns.
 - id is primary key in database generated by database.
 - Name is name of web user.
 - time_login is the time that database get the informations (user has booked).
 - tel is telephone number of user.
 - num_people is number of guest that user has sent to restaurant.
 - queue is queue number that program has generated for user.
 - status is the status of user. It has 4 status. WFA = Waiting for admin, CBC = Cancel by client, CBA Cancel by admin, DON = Done.
 - time_status is the time that user has been finished all process such as CBC, DON or CBA.
 - c_id is client ID that from user, generated by UUID in web frontend side.
