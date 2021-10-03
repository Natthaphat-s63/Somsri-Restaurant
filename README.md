# Week2
The main goals of this week are:
  - Generating queue number and save it into database.
  - Calculate the remaining queue.
  - Sending queue number and remaining queue back to frontend.

# Major change
  - Delete reserve time in both frontend and backend including in database.
# Report for Frontend
Redesign the UI to make it show queue number and remianing number (Does not implementing pub and sub in this frontend) and make it work on both in computer and mobile.
And we decide to delete the reserve time because it's not in our concept work.

<img src="frontend2.png">

# Report for Backend
**Queueing system**

We can generate queue number by using the recent queue in database then add 1 to it but right now we can't reset the queue when the data skip the day.
And we can't calculate the remaining queue right now.

**Broker**

We use shell script to open and close mqtt broker when the program open broker will open too and when the program close broker will close too.
But the problem about using shell script is it require full path to the shell script file.

**GUI**

We decide to create 2 tabs, the first tab is about handling queue in a day and the second tab is about inspect the datas in database.

*First tab*:

<img src = "backendgui2_1.png">

In this tab, we read the data specific for the day we open the program. It can display the current data that already update into database without to mannually refresh but if there are some datas that's already in the database with the same date as the day you open the program, you can use refresh button to show them all.
And you can search by queue number to find the data of that queue by tying queue number in search entry and click search.

<img src = "backendgui2_2.png">

*Second tab*:

<img src = "backendgui2_3.png">

In this tab, like the first tab but it can only inspect the data by searching by Name,Date,Tel,Number of people (right now search by date can't use). If you do not enter anything in search entry and click search button, it'll show all the data in database.

<img src = "backendgui2_4.png">

<img src = "backendgui2_5.png">

**Conclusion**


