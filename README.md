90# Week1
The main goals of this week are:
  - Implementing mosquitto broker setup to create our own broker that enabled websocket function.
  - Create frontend of the website to sending some JSON data for testing.
  - Connecting the frontend, backend and database by sending some JSON from frontend to backend.
  
# Report for Frontend
We decide to use Reactjs as a library for frontend development because it make for building single page application that suit to our project well.And we are using MQTT.js module that already support MQTT over Websocket so it make things easy to implement because we just specify in connect option that we want to use websocket and change the URL to websocket protocal(Ex. ws://localhost:port).We are using the form tag to create form and submit it. Then change the data to JSON data and change it to string(mqtt only support sending messages) then publish it through mqtt.

the result:

<img src="frontend1.png">

# Report for Backend
**Set up mosquitto broker**

We decide to setup our own broker to keep this project as private.

Step 1: Install msoquitto following this instruction :http://www.steves-internet-guide.com/install-mosquitto-linux/  *Note: we are using broker on linux

Step 2: Go to /etc/mosquitto/conf.d and then create file name default.conf that include this command

  - listener 9001
  - protocol websocket
  - listener 1883
  - protocol mqtt

Then save the file. So now when we want to open the broker  use this command in terminal.

  - mosquitto -c /etc/mosquitto/conf.d/default.conf

Then the terminal will look like this.


