//gcc -Wall backend.c -o b -lpaho-mqtt3c -ljson-c -lmysqlclient -lpthread `pkg-config --cflags gtk+-3.0` `pkg-config --libs gtk+-3.0` 
//mosquitto -c /etc/mosquitto/conf.d/default.conf

#include <json-c/json.h>
#include <json-c/json_inttypes.h>
#include <json-c/json_object.h>
#include <json-c/json_tokener.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <mysql/mysql.h>
#include "MQTTClient.h"

#define NUM_JSON_ELEMENT     4
#define ADDRESS             "ws://localhost:9001"
#define CLIENTID            "ExampleClientPub"
#define TOPIC               "request/#"
#define QOS                  2
#define TIMEOUT              10000L

GtkApplication  *app;
GtkTreeStore    *store, *store2;
GtkTreeIter      iter1, iter2;
GtkWidget       *window, *notebook, *tab1, *tab2, *Brf /*Refresh Button*/;
GtkWidget       *Entryq, *Entryday, *Entrymonth, *Entryyear,
                *Entryname, *Entrynumpeople ,*Entrytel, *Entrystatus;
GtkWidget       *listbox, *listbox2;

gchar   *text;
gchar   *selected;
int      state,status;      //refresh protection
int      statesel = 0;      //stateselection (for pop-up protection)
int      first_time = 1;
char     date[11];
int canc_count =0;
enum
{
  COL_QUEUE,
  COL_ID, 
  COL_NAME,
  COL_NUMBER,
  COL_TEL,
  COL_STATUS,
  COL_TISTA,
  COL_TIMLOG,
  COL_COUNTS
};

struct res_obj
{
  char* c_id;
  int q_num;
  int r_num;
  int sent ;
};

struct res_obj respond;
MQTTClient client;

void myCSS(void);

static pthread_t chk_almost; 
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static gboolean btn_clicked(GtkWidget *widget, gpointer parent);        //Create a pop-up when you want to exit the program.
static gboolean conf_clicked(GtkWidget *widget, gpointer parent);       //Create a pop-up when you want to confirm this person.
static gboolean canc_clicked(GtkWidget *widget, gpointer parent);       //Create a pop-up when you want to cancle this person.  
void activate( GtkApplication *app, gpointer user_data );               //Create widgets in the application
void view_selected(GtkTreeSelection *sel, gpointer data);               //Returns datas in that row.
void searchbutton_callback(GtkWidget *b, gpointer data);                //Find customer data of that queue today.
void confirmbutton_callback(GtkWidget *b, gpointer data);               //Verify that customer to change the status to DON.
void cancbutton_callback(GtkWidget *b, gpointer data);                  //Cancel that customer to change the status to CBA.
void refbutton_callback(GtkWidget *b, gpointer data);                   //Refresh all customer data today
void search2button_callback(GtkWidget *b, gpointer data);               //Search for customer information in the database. (tab2)


void finish_with_error(MYSQL *con)
{
  fprintf(stderr, "%s\n", mysql_error(con));
  mysql_close(con);
  exit(1);
}

void send_status(char* id ,char*stat)
{
  printf("%s",stat);
  MQTTClient_message pubmsg = MQTTClient_message_initializer;
  char t_opic[50];
  char payload[10];

  sprintf(t_opic,"respond/%s",id);
  printf("id = %s\n",id);
  sprintf(payload,"%s",stat);

  pubmsg.payload = payload;
  pubmsg.payloadlen = strlen(payload);
  pubmsg.qos = QOS;
  pubmsg.retained = 0;

  MQTTClient_publishMessage(client,t_opic,&pubmsg,NULL);
}

void send_res(char* id,int Q,int R)
{

  MQTTClient_message pubmsg = MQTTClient_message_initializer;
  char t_opic[50];
  char payload[50];

  sprintf(t_opic,"respond/%s",id);
  sprintf(payload,"{\"queue\":\"%d\",\"remain\":\"%d\"}",Q,R);

  pubmsg.payload = payload;
  pubmsg.payloadlen = strlen(payload);
  pubmsg.qos = QOS;
  pubmsg.retained = 0;

  MQTTClient_publishMessage(client,t_opic,&pubmsg,NULL);
 
}

void send_recent_remaining(int R)
{
  MQTTClient_message pubmsg = MQTTClient_message_initializer;
  char payload[20];
  if(R==0)R=-1;
  sprintf(payload,"{\"recent_remaining\":\"%d\"}",R+1); 
  pubmsg.payload = payload;
  pubmsg.payloadlen = strlen(payload);
  pubmsg.qos = QOS;
  pubmsg.retained = 0;
  MQTTClient_publishMessage(client,"recent_remaining",&pubmsg,NULL);
}

int remaining(int Q)
{
    int remain=0;
    MYSQL *con = mysql_init(NULL);
    if (con == NULL)
    {	
        fprintf(stderr, "%s\n", mysql_error(con));
        return;
    }
     if (mysql_real_connect(con, "localhost", "root", "password","mydb", 0, NULL, 0) == NULL)finish_with_error(con);
    char *query = malloc(120);
    sprintf(query,"SELECT queue FROM queue WHERE DATE(time_login) = '%s' AND status = 'WFA'",date);

    if (mysql_query(con, query))finish_with_error(con);
    MYSQL_RES *result = mysql_store_result(con);

    if (mysql_num_rows(result)==0)
    {
        printf("\nEmpty result\n");
        return;
    }

    int num_fields = mysql_num_fields(result);

    if(num_fields<1){
            printf("no fields returned");
            finish_with_error(con);
    }

    int count = 0;
    char q_str[10] ;
    sprintf(q_str,"%d",Q);
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(result)))
    {
        if(strcmp(q_str,row[0])==0)break;
        count++;
    }

    mysql_free_result(result);
    remain = count;
    free(query);
    mysql_close(con);
    return(remain);
}

void *Thread_job()
{
    pthread_mutex_lock(&lock);
    pthread_cond_wait(&cond,&lock);
    while(1)
    {
        usleep(10000);
        MYSQL *con = mysql_init(NULL);
        if (con == NULL)
        {	
        fprintf(stderr, "%s\n", mysql_error(con));
        return ;
        }
        if (mysql_real_connect(con,"localhost", "root", "password","mydb", 0, NULL, 0) == NULL)finish_with_error(con);
        char *query = malloc(120);
        sprintf(query,"SELECT queue,c_id,tel FROM queue WHERE DATE(time_login) = '%s' AND status = 'WFA'",date);
        if (mysql_query(con, query))finish_with_error(con);
        MYSQL_RES *result = mysql_store_result(con);
        if (mysql_num_rows(result)==0)
        {
          send_recent_remaining(0);  
          mysql_free_result(result);
          mysql_close(con);
          continue;
        }  
        int num_fields = mysql_num_fields(result);
        if(num_fields<1){
        printf("no fields returned");
        finish_with_error(con);
        }
        MYSQL_ROW row;
        int size = mysql_num_rows(result);
        int* remain_recorder = malloc(size*sizeof(int));
        int* queue_recorder = malloc(size*sizeof(int));
        memset(remain_recorder, -1, size*sizeof(int)); 
        memset(queue_recorder, -1, size*sizeof(int));
        int i =0;
        while (i<size)
        {
            row = mysql_fetch_row(result);
            int remain = remaining(atoi(row[0]));

            queue_recorder[i]=atoi(row[0]);
            if(remain_recorder[i] != remain) // if remain updated
            {
                //for realtime specific
                send_res(row[1],atoi(row[0]),remain);
            }
            remain_recorder[i] = remain;
            i++;
        }
        send_recent_remaining(remain_recorder[size-1]);          
        free(remain_recorder);
        free(queue_recorder);
        free(query);
        mysql_free_result(result);
        mysql_close(con);
    }
    pthread_mutex_unlock(&lock);
    return;
}

void cancelbyclient(char* id)
{
    MYSQL *con = mysql_init(NULL);
    
    if (con == NULL)
    {
        fprintf(stderr, "mysql_init() failed\n");
        exit(1);
    }
     if (mysql_real_connect(con, "localhost", "root", "password","mydb", 0, NULL, 0) == NULL)
    {
        finish_with_error(con);
    }

    char *query = malloc(120);
    sprintf(query,"UPDATE queue SET status='CBC' WHERE c_id = '%s'",id);
    if (mysql_query(con, query))
    {
        finish_with_error(con);
    }
    free(query);
    gtk_entry_set_placeholder_text(GTK_ENTRY(Entryq),"Please Refresh Client has cancelled !!!");
    mysql_close(con);
    
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    char* obj [NUM_JSON_ELEMENT];
    const char* key [NUM_JSON_ELEMENT] = {"Name","tel","num_people","c_id"};
    MYSQL *con = mysql_init(NULL);
    if (con == NULL)
    {    
        fprintf(stderr, "%s\n", mysql_error(con));
        return 0;
    }
    if (mysql_real_connect(con, "localhost", "root", "password","mydb", 0, NULL, 0) == NULL)finish_with_error(con);
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
        
    char* payloadptr = message->payload;
    printf("%s\n",payloadptr);
    if (strcmp(payloadptr,"Disconnect unexpected")==0)
    {
        MQTTClient_freeMessage(&message);
        MQTTClient_free(topicName);
    	return 1;
    }
    else if(payloadptr[0] == 'C')
    {
        int len  = (int)strlen(payloadptr);
        char cid[len-2];
        for(int i = 2;i<len;i++)
        {
            cid[i-2] = payloadptr[i];
        }
        canc_count++;
        cancelbyclient(cid);
        return 1;
    }
    json_object *json_obj = json_tokener_parse(payloadptr);         
    for(int i =0 ; i < NUM_JSON_ELEMENT ; i++)
    {
        json_object *dummy;
        json_object_object_get_ex(json_obj, key[i], &dummy);
        obj[i]=json_object_get_string(dummy);
    }
  //INSERT DATA
    char *query = malloc(150);
    sprintf(query,"SELECT queue FROM queue WHERE DATE(time_login) = '%s' ORDER BY queue DESC LIMIT 1 ",date);
    if (mysql_query(con, query))finish_with_error(con);
    int recent_queue=1; 
    MYSQL_ROW row;
    MYSQL_RES *result = mysql_store_result(con);
    if (strcmp(result,"") != 0)
    {
        row = mysql_fetch_row(result); 
        recent_queue =atoi(row[0]);
        recent_queue++; // for next queue
    }
    mysql_free_result(result);
    memset(query,0,strlen(query));
    sprintf(query,"INSERT INTO queue (Name,tel,num_people,queue,status,c_id) VALUES('%s','%s','%s','%d','WFA','%s')",obj[0],obj[1],obj[2],recent_queue,obj[3]);
    if (mysql_query(con, query)) finish_with_error(con);
  
    int remain =  remaining(recent_queue);
    respond.c_id = obj[3];
    respond.q_num = recent_queue;
    respond.r_num = remain;
    respond.sent = 1;

    memset(query,0,strlen(query));
    sprintf(query,"SELECT * FROM queue ORDER BY id DESC LIMIT 1 ");
    if (mysql_query(con, query))finish_with_error(con);
    result = mysql_store_result(con);

    if (mysql_num_rows(result)==0)
    {
        printf("\nEmpty result\n");
        return;
    }

    MYSQL_ROW row1;
    row1 = mysql_fetch_row(result); 
    //for auto refresh
    mysql_free_result(result);
    if (state == 0){

        char recentq[10]; 
        sprintf(recentq,"%d",recent_queue);

        gtk_tree_store_append(store, &iter1, NULL);
        gtk_tree_store_set(store, &iter1,
            COL_ID,  row1[0], 
            COL_NAME, row1[1], 
            COL_QUEUE, row1[5], 
            COL_NUMBER, row1[4],
            COL_TEL,row1[3],
            COL_STATUS,row1[6],
            COL_TISTA ,row1[7], -1);

    }

    free(query);
    mysql_close(con);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause) 
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}




int main(int argc, char **argv)
{
    respond.q_num =0;
    respond.r_num =0;
    respond.sent =0;
    mysql_library_init( 0, NULL, NULL );
    if(pthread_mutex_init(&lock,NULL))
    {        
        printf("Lock init Failed!!\n");
        exit(1);
    }
    if(pthread_cond_init(&cond,NULL))
    {
        printf("Cond init Failed!!\n");
        exit(1);
    }
    mysql_library_init(0,NULL,NULL);
    int retval = pthread_create(&chk_almost, NULL,Thread_job,NULL );
    printf( "Create thread %s\n", retval ? "FAILED" : "OK" ); 
    if(retval) exit(1);
    time_t rawt = time(NULL);
    struct tm  *time = localtime(&rawt);
    sprintf(date,"%d-%02d-%02d",time->tm_year + 1900,time->tm_mon + 1,time->tm_mday);
    MQTTClient_create(&client,ADDRESS,CLIENTID,MQTTCLIENT_PERSISTENCE_NONE,NULL);
    MQTTClient_setCallbacks(client,NULL,connlost,msgarrvd,NULL);
    int rc;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    if((rc = MQTTClient_connect(client,&conn_opts))!=MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n",rc);
        exit(-1);
    }
        //start multithread after connect to broker	
    pthread_cond_signal(&cond);
    app = gtk_application_new( NULL, G_APPLICATION_FLAGS_NONE ); 
    //create application
    g_signal_connect( app, "activate", G_CALLBACK(activate), NULL );   
    g_application_run( G_APPLICATION(app), argc, argv );
    g_object_unref( app );

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    if (chk_almost) pthread_join( chk_almost, NULL );
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&lock);
    return 0;
}

static gboolean btn_clicked(GtkWidget *widget, gpointer parent)
{
    GtkWidget *dialog;
    dialog = gtk_message_dialog_new(GTK_WINDOW(parent),     //create popup window
                GTK_DIALOG_DESTROY_WITH_PARENT, 
                GTK_MESSAGE_QUESTION, 
                GTK_BUTTONS_YES_NO, 
                "Do you want to exit this program?");
    gtk_window_set_title(GTK_WINDOW(dialog), "Exit program");
    int result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    if (result == GTK_RESPONSE_YES)
    {
        MQTTClient_disconnect(client, 10000);
        pthread_cancel(chk_almost);
        gtk_main_quit();
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

static gboolean conf_clicked(GtkWidget *widget, gpointer parent)
{
    if(statesel == 0 )return TRUE;
    GtkWidget *dialog;
    dialog = gtk_message_dialog_new(GTK_WINDOW(parent),     //create popup window
                GTK_DIALOG_DESTROY_WITH_PARENT, 
                GTK_MESSAGE_QUESTION, 
                GTK_BUTTONS_YES_NO, 
                "Do you want to confirm this person?");
    gtk_window_set_title(GTK_WINDOW(dialog), "Confirm customer");
    int result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    if (result == GTK_RESPONSE_YES)
    {
        confirmbutton_callback(NULL,NULL);
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

static gboolean canc_clicked(GtkWidget *widget, gpointer parent)
{
    if(statesel == 0 )return TRUE;
    GtkWidget *dialog;
    dialog = gtk_message_dialog_new(GTK_WINDOW(parent),     //create popup window
                GTK_DIALOG_DESTROY_WITH_PARENT, 
                GTK_MESSAGE_QUESTION, 
                GTK_BUTTONS_YES_NO, 
                "Do you want to cancle this person?");
    gtk_window_set_title(GTK_WINDOW(dialog), "Cancle customer");
    int result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    
    if (result == GTK_RESPONSE_YES)
    {
        cancbutton_callback(NULL,NULL);
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

void view_selected(GtkTreeSelection *sel, gpointer data)
{
    GtkTreeIter iter;
    GtkTreePath *sel_path;
    GtkTreeModel *model;
    

    if (gtk_tree_selection_get_selected(sel, &model, &iter))
    {
        const gchar *QUEUE, *NAME, *NUMBER, *TEL, *STATUS, *ID, *TISTA;

        gtk_tree_model_get(model, &iter, COL_ID, &ID, -1);        //Gets the value displayed for each column in the same row. 
        gtk_tree_model_get(model, &iter, COL_QUEUE, &QUEUE, -1);
        gtk_tree_model_get(model, &iter, COL_NAME, &NAME, -1);
        gtk_tree_model_get(model, &iter, COL_NUMBER, &NUMBER, -1);
        gtk_tree_model_get(model, &iter, COL_TEL, &TEL, -1);
        gtk_tree_model_get(model, &iter, COL_STATUS, &STATUS, -1);
        gtk_tree_model_get(model, &iter, COL_TISTA, &TISTA, -1);
        MYSQL *con = mysql_init(NULL);
        if (con == NULL)
        {
            fprintf(stderr, "mysql_init() failed\n");
            exit(1);
        }
        if (mysql_real_connect(con, "localhost", "root", "password","mydb", 0, NULL, 0) == NULL)
        {
            finish_with_error(con);
        }
        char *query = malloc(120);
        sprintf(query,"SELECT status FROM queue WHERE DATE(time_login) = '%s' AND queue = %d AND status = 'WFA'",date,atoi(QUEUE));
        if (mysql_query(con,query))
        {
            finish_with_error(con);
        }   
        free(query);
        MYSQL_RES *result = mysql_store_result(con); //all data


        if(mysql_num_rows(result)!=0)
        {
            selected = QUEUE;
            statesel = 1;
        }
        else
        {
            statesel = 0;
        }

        printf("\tQueue: %s, Name: %s, Number of People: %s\n", QUEUE, NAME, NUMBER);
        printf("\tID : %s, Status : %s\n\n", selected,STATUS);
    

    }
}

void searchbutton_callback(GtkWidget *b, gpointer data)
{
    
    const gchar *queue_id = gtk_entry_get_text(GTK_ENTRY(Entryq));    //Gets the value that is in the id entry.

    gtk_tree_store_clear(store);  //clear all liststore

    MYSQL *con = mysql_init(NULL);
    if (con == NULL)
    {
        fprintf(stderr, "mysql_init() failed\n");
        exit(1);
    }
    if (mysql_real_connect(con, "localhost", "root", "password","mydb", 0, NULL, 0) == NULL)
    {
        finish_with_error(con);
    }

    char *query = malloc(120);
    sprintf(query,"SELECT id,Name,time_login,tel,num_people,queue,status,time_status FROM queue WHERE DATE(time_login) = '%s'",date);
    if (mysql_query(con,query))
    {
        finish_with_error(con);
    }
    free(query);
    MYSQL_RES *result = mysql_store_result(con); //all data

    if (mysql_num_rows(result)==0)
    {
        printf("\nEmpty result\n");
        return;
    }

    int num_fields = mysql_num_fields(result); //len column
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) //use data in row
    { 
        if(strcmp(row[5],queue_id) == 0){       //Match the same ID sent in the entry.

            gtk_tree_store_append(store, &iter1, NULL);   //create liststore
            gtk_tree_store_set(store, &iter1,
            COL_NAME, row[1], 
            COL_QUEUE, row[5], 
            COL_NUMBER, row[4],
            COL_TEL,row[3],
            COL_STATUS,row[6],
            COL_TISTA ,row[7], -1);
            state = 1;      //change the stage (It is there to prevent refresh while searching for id)
        }
        else
        {
            continue;
        }    
    }
    statesel = 0;
    //printf("hi\t%d\n",statesel);

}

void search2button_callback(GtkWidget *b, gpointer data){

    char *query = malloc(700);
    int count = 0;
    char date2[11];

    char *name2 = gtk_entry_get_text(GTK_ENTRY(Entryname));
    char *tel2 = gtk_entry_get_text(GTK_ENTRY(Entrytel));
    char *numpeople2 = gtk_entry_get_text(GTK_ENTRY(Entrynumpeople));
    char *day = gtk_entry_get_text(GTK_ENTRY(Entryday));
    char *month = gtk_entry_get_text(GTK_ENTRY(Entrymonth));
    char *year = gtk_entry_get_text(GTK_ENTRY(Entryyear));
    char *status2 = gtk_entry_get_text(GTK_ENTRY(Entrystatus));

    
    sprintf(date2,"1970-01-01");

    if(strcmp(day,"")!=0 || strcmp(month,"")!=0 || strcmp(year,"")!=0)
    {

        if(strcmp(day,"")!=0 && strcmp(month,"")!=0 && strcmp(year,"")!=0)
        {
            sprintf(date2,"%s-%s-%s",year,month,day);
        }
        else
        {
            gtk_entry_set_text(Entryday,"");
            gtk_entry_set_text(Entrymonth,"");
            gtk_entry_set_text(Entryyear,"");
        }
    }

    gtk_tree_store_clear(store2);
    MYSQL *con = mysql_init(NULL);

    if (con == NULL)
    {
        fprintf(stderr, "mysql_init() failed\n");
        exit(1);
    }
     if (mysql_real_connect(con, "localhost", "root", "password","mydb", 0, NULL, 0) == NULL)
    {
        finish_with_error(con);
    }
    if(strcmp(name2,"")==0){
        count += 1;
    }
    if(strcmp(tel2,"")==0){
        count += 1;
    }
    if(strcmp(numpeople2,"")==0){
        count += 1;
    }
    if(strcmp(date2,"1970-01-01")==0){
        count += 1;
    }
    if(strcmp(status2,"")==0){
        count += 1;
    }

    if (count == 5)
    {
        sprintf(query,"SELECT  * FROM queue");
    }
    else if (count == 4)
    {
        sprintf(query,"SELECT  * FROM queue WHERE Name = '%s' OR tel = '%s' OR num_people = %d OR DATE(time_login) = '%s' OR status = '%s'",name2,tel2,atoi(numpeople2),date2,status2);
    }
    else if (count == 3)
    {
        sprintf(query,"SELECT  * FROM queue WHERE (Name = '%s' AND tel = '%s') OR (Name = '%s' AND num_people = %d) OR (tel = '%s'  AND num_people = %d) OR (Name = '%s' AND DATE(time_login) = '%s') OR (tel = '%s' AND DATE(time_login) = '%s') OR (num_people = %d AND DATE(time_login) = '%s') OR (Name = '%s' AND status = '%s') OR (tel = '%s'  AND status = '%s') OR (num_people = %d AND status = '%s') OR (DATE(time_login) = '%s' AND status = '%s')",name2,tel2,name2,atoi(numpeople2),tel2,atoi(numpeople2),name2,date2,tel2,date2,atoi(numpeople2),date2,name2,status2,tel2,status2,atoi(numpeople2),status2,date2,status2);
    }
    else if (count == 2)
    {
        sprintf(query,"SELECT  * FROM queue WHERE (Name = '%s' AND tel = '%s' AND num_people = %d) OR (Name = '%s' AND tel = '%s' AND DATE(time_login) = '%s') OR (Name = '%s' AND DATE(time_login) = '%s' AND num_people = %d) OR (DATE(time_login) = '%s' AND tel = '%s' AND num_people = %d) OR (Name = '%s' AND tel = '%s' AND status = '%s') OR (Name = '%s' AND num_people = %d AND status = '%s') OR (Name = '%s' AND DATE(time_login) = '%s' AND status = '%s') OR (tel = '%s' AND num_people = %d AND status = '%s' OR (tel = '%s' AND DATE(time_login) = '%s' AND status = '%s') OR (num_people = %d AND DATE(time_login) = '%s' AND status = '%s'))",name2,tel2,atoi(numpeople2),name2,tel2,date2,name2,date2,atoi(numpeople2),date2,tel2,atoi(numpeople2),name2,tel2,status2,name2,atoi(numpeople2),status2,name2,date2,status2,tel2,atoi(numpeople2),status2,tel2,date2,status2,atoi(numpeople2),date2,status2);
    }
    else if (count == 1)
    {
        sprintf(query,"SELECT  * FROM queue WHERE (Name = '%s' AND tel = '%s' AND num_people = %d AND DATE(time_login) = '%s') OR (Name = '%s' AND tel = '%s' AND num_people = %d AND status = '%s') OR (Name = '%s' AND tel = '%s' AND DATE(time_login)= '%s' AND status = '%s') OR (Name = '%s'AND num_people = %d AND DATE(time_login) = '%s' AND status = '%s') OR (tel = '%s' AND num_people = %d AND DATE(time_login) = '%s' AND status = '%s')",name2,tel2,atoi(numpeople2),date2,name2,tel2,atoi(numpeople2),status2,name2,tel2,date2,status2,name2,atoi(numpeople2),date2,status2,tel2,atoi(numpeople2),date2,status2);
    }
    else if (count == 0)
    {
        sprintf(query,"SELECT  * FROM queue WHERE Name = '%s' AND tel = '%s' AND num_people = %d AND DATE(time_login) = '%s' AND status = '%s'",name2,tel2,atoi(numpeople2),date2,status2);
    }

    if (mysql_query(con, query))
    {
        finish_with_error(con);
    }

    MYSQL_RES *result = mysql_store_result(con); //all data

    if (mysql_num_rows(result)==0)
    {
        printf("\nEmpty result\n");
        return;
    }

    int num_fields = mysql_num_fields(result); //len column
    MYSQL_ROW row;
    
    while ((row = mysql_fetch_row(result))) //use data in row
    { 
        gtk_tree_store_append(store2, &iter2, NULL);
        gtk_tree_store_set(store2, &iter2,
        COL_TIMLOG, row[2], 
        COL_NAME, row[1], 
        COL_QUEUE, row[5], 
        COL_NUMBER, row[4],
        COL_TEL,row[3],
        COL_STATUS,row[6],
        COL_TISTA ,row[7], -1);
    }

    state = 0;
    free(query);
    mysql_free_result(result);
    mysql_close(con);
}

void confirmbutton_callback(GtkWidget *b, gpointer data)
{
    if(selected==NULL) return;
    MYSQL *con = mysql_init(NULL);
    
    if (con == NULL)
    {
        fprintf(stderr, "mysql_init() failed\n");
        exit(1);
    }
     if (mysql_real_connect(con, "localhost", "root", "password","mydb", 0, NULL, 0) == NULL)
    {
        finish_with_error(con);
    }

    int sel = atoi(selected);
    char *query = malloc(256);
    sprintf(query,"UPDATE queue SET status='DON' WHERE queue = %d AND status = 'WFA' AND DATE(time_login) = '%s'",sel,date);
    if (mysql_query(con, query))
    {
        finish_with_error(con);
    }
    sprintf(query,"SELECT c_id FROM queue WHERE queue = %d AND status = 'DON' AND DATE(time_login) = '%s'",sel,date);
    if (mysql_query(con, query))
    {
            finish_with_error(con);
    }
    MYSQL_RES *result = mysql_store_result(con);
    if (mysql_num_rows(result)==0)
    {
        printf("\nEmpty result\n");
        return;
    }
    MYSQL_ROW row;
    row = mysql_fetch_row(result); 
    printf("%s",row[0]);
    send_status(row[0],"DON");
    free(query);
    mysql_free_result(result);
    mysql_close(con);
    refbutton_callback(b,NULL);
}

void cancbutton_callback(GtkWidget *b1, gpointer data){
 
  if(selected==NULL) return;
  MYSQL *con = mysql_init(NULL);
  if (con == NULL)
  {
      fprintf(stderr, "mysql_init() failed\n");
      exit(1);
  }
   if (mysql_real_connect(con, "localhost", "root", "password","mydb", 0, NULL, 0) == NULL)
  {
      finish_with_error(con);
  }

  int sel = atoi(selected);
  char *query = malloc(256);
  sprintf(query,"UPDATE queue SET status='CBA' WHERE queue = %d AND status = 'WFA' AND DATE(time_login) = '%s'",sel,date);

  if (mysql_query(con, query))
  {
        finish_with_error(con);
  }
  sprintf(query,"SELECT c_id FROM queue WHERE queue = %d AND status = 'CBA' AND DATE(time_login) = '%s'",sel,date);
  if (mysql_query(con, query))
  {
        finish_with_error(con);
  }

  MYSQL_RES *result = mysql_store_result(con);

  if (mysql_num_rows(result)==0)
  {
    printf("\nEmpty result\n");
    return;
  }

  MYSQL_ROW row;
  row = mysql_fetch_row(result); 
  send_status(row[0],"CBA");
  free(query);
  mysql_free_result(result);
  mysql_close(con);
  refbutton_callback(b1,NULL);
}

void refbutton_callback(GtkWidget *b1,gpointer data)
{
    statesel = 0;
    canc_count = 0;
    gtk_entry_set_placeholder_text(GTK_ENTRY(Entryq),"");
    gtk_tree_store_clear(store);
    MYSQL *con = mysql_init(NULL);

    if (con == NULL)
    {
        fprintf(stderr, "mysql_init() failed\n");
        exit(1);
    }
     if (mysql_real_connect(con, "localhost", "root", "password","mydb", 0, NULL, 0) == NULL)
    {
        finish_with_error(con);
    }

    char *query = malloc(120);
    sprintf(query,"SELECT * FROM queue WHERE DATE(time_login) = '%s' AND status = 'WFA'",date);
    if (mysql_query(con, query))finish_with_error(con);
    MYSQL_RES *result = mysql_store_result(con); //all data

    if (mysql_num_rows(result)==0)
    {
        printf("\nEmpty result\n");
        return ;
    }

    int num_fields = mysql_num_fields(result); //len column
    MYSQL_ROW row;
    
    while ((row = mysql_fetch_row(result))) //use data in row for show every row
    { 

        gtk_tree_store_append(store, &iter1, NULL);
        gtk_tree_store_set(store, &iter1,
        COL_ID,  row[0], 
        COL_NAME, row[1], 
        COL_QUEUE, row[5], 
        COL_NUMBER, row[4],
        COL_TEL,row[3],
        COL_STATUS,row[6],
        COL_TISTA ,row[7], -1);
    }

    state = 0;
    free(query);
    mysql_free_result(result);
    mysql_close(con);

}
void activate( GtkApplication *app, gpointer user_data ){

    GtkWidget *scrwin, *frame, *vbox, *hbox, *eventbox, *vbox0, *vbox00, *hbox0, *labelid;
    GtkWidget *Bcc /*Cancel Button*/, *Bex /*Exit Button*/,
              *Bs /*Search Button*/, *Bcf /*Confirm Button*/;

    MQTTClient_subscribe(client, TOPIC, QOS);
    //publish outside callback to avoid deadlock
    if(respond.sent)
    {
      send_res(respond.c_id,respond.q_num, respond.r_num);
      respond.q_num =0;
      respond.r_num =0;
      respond.sent =0;
    }
    
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);                                                 //create window
    gtk_window_set_title(GTK_WINDOW(window), "Somsri Restaurant");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_window_fullscreen(window);
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(btn_clicked), (gpointer) window);    //If selected, press the destroy button on the top right.

    vbox00 = gtk_vbox_new (FALSE, 0);                                                             //Vertical box, if added later will be added below.
    gtk_container_add (GTK_CONTAINER (window), vbox00);                                           //Put this box in Windows

    notebook = gtk_notebook_new ();                                                               //create notebook
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);                              //set tap position (top)
    gtk_box_pack_start (GTK_BOX (vbox00), notebook, TRUE, TRUE, 0);                               //put this widgets in this box

    tab1 = gtk_label_new ("Tab1");
    gtk_widget_show (tab1);

    vbox0 = gtk_vbox_new(FALSE, 0);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox0 , tab1);                             //add vbox0 in tab1

    hbox0 = gtk_hbox_new(FALSE, 5);                                                               //Horizontal box, if add widget, it will add right side.
    gtk_box_pack_start(GTK_BOX(vbox0), hbox0, TRUE, TRUE, 5);

    labelid = gtk_label_new("Enter Queue: ");
    gtk_box_pack_start(GTK_BOX(hbox0), labelid, TRUE, TRUE, 5);

    Entryq = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox0), Entryq, TRUE, TRUE, 5);
    
   

    Bs = gtk_button_new_with_label("Search");
    gtk_widget_set_can_focus(Bs, FALSE);                                                            //Specifies whether widget can own the input focus
    gtk_widget_set_size_request(Bs, 40, 40);
    gtk_box_pack_start(GTK_BOX(hbox0), Bs, TRUE, TRUE, 3);
    g_signal_connect(Bs,"clicked",G_CALLBACK(searchbutton_callback),NULL /*data callback*/);        //If a button is pressed, the callback function is activated.

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox0), hbox, TRUE, TRUE, 0);

    frame = gtk_frame_new("Data");
    gtk_widget_set_size_request(frame, 1000, 800);

    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(frame), eventbox);
    gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE,TRUE,0);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

    scrwin = gtk_scrolled_window_new (NULL, NULL);  
    gtk_widget_set_hexpand (scrwin, TRUE);                                                          //Sets whether the widget would like any available extra horizontal space. When a user resizes a GtkWindow, widgets with expand=TRUE generally receive the extra space.
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrwin), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_container_add (GTK_CONTAINER (eventbox), scrwin);                                           //add scrollbar to eventbox 

    listbox = gtk_list_box_new();
    gtk_list_box_set_selection_mode (GTK_LIST_BOX (listbox), GTK_SELECTION_NONE);                   //You can select a value in the listbox
    gtk_container_add (GTK_CONTAINER (scrwin), listbox);

    /* Create store */
    store = gtk_tree_store_new(
      COL_COUNTS, 
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,    
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING);

    /* Creaete tree to contain store and keep tree widget to win */
    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_container_add(GTK_CONTAINER(listbox), tree);
    
    /* Renderer data to view */
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    renderer = gtk_cell_renderer_text_new();                                //widgets are used to display information within widgets
    column = gtk_tree_view_column_new_with_attributes(                      //create column
        "           QUEUE           ", 
        renderer, 
        "text", COL_QUEUE, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);               //append the column

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "           NAME            ", 
        renderer, 
        "text", COL_NAME, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "       NUMBER of PEOPLE        ", 
        renderer, 
        "text", COL_NUMBER, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "       Telephone Number        ", 
        renderer, 
        "text", COL_TEL, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "           STATUS          ", 
        renderer, 
        "text", COL_STATUS, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
    
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    gtk_tree_selection_set_mode(GTK_TREE_SELECTION(sel), GTK_SELECTION_SINGLE);
    g_signal_connect(G_OBJECT(sel), "changed", G_CALLBACK(view_selected), NULL);

    Bcf = gtk_button_new_with_label("Confirm");
    gtk_widget_set_can_focus(Bcf, FALSE);
    gtk_widget_set_size_request(Bcf, 64, 64);
    gtk_box_pack_start(GTK_BOX(vbox), Bcf, TRUE, TRUE, 0);
    g_signal_connect(Bcf,"clicked",G_CALLBACK(conf_clicked),NULL);


    Brf = gtk_button_new_with_label("Refresh");
    gtk_widget_set_can_focus(Brf, TRUE);
    gtk_widget_set_size_request(Brf, 64, 64);
    gtk_box_pack_start(GTK_BOX(vbox), Brf, TRUE, TRUE, 0);
    g_signal_connect(Brf,"clicked",G_CALLBACK(refbutton_callback),NULL); 


    Bcc = gtk_button_new_with_label("Cancel");
    gtk_widget_set_can_focus(Bcc, FALSE);
    gtk_widget_set_size_request(Bcc, 64, 64);
    gtk_box_pack_start(GTK_BOX(vbox), Bcc, TRUE, TRUE, 0);
    g_signal_connect(Bcc,"clicked",G_CALLBACK(canc_clicked),NULL);

    Bex = gtk_button_new_with_label("Exit");
    gtk_widget_set_can_focus(Bex, FALSE);
    gtk_widget_set_size_request(Bex, 64, 64);
    gtk_box_pack_start(GTK_BOX(vbox), Bex, TRUE, TRUE, 0);
    g_signal_connect(G_OBJECT(Bex), "clicked", G_CALLBACK(btn_clicked), (gpointer) window);


    /*   tab2   */

    GtkWidget *scrwin2, *frame2, *vbox2, *hboxlabel, *hboxentry, *eventbox2, *listbox2, *hboxdate, *hboxend,
              *labeldate, *labelname, *labelnumpeople, *labeltel, *labelstatus, *nulllabel, *null2label;
    GtkWidget *Bs2 /*Search Button*/, *Bex2 /*Exit Button*/;

    tab2 = gtk_label_new ("Tab2");
    gtk_widget_show (tab2);

    vbox2 = gtk_vbox_new(FALSE, 5);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox2 , tab2);


    hboxlabel = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox2), hboxlabel, TRUE, TRUE, 5);

    labeldate = gtk_label_new("Date");
    gtk_label_set_xalign (labeldate, 0.0);
    gtk_box_pack_start(GTK_BOX(hboxlabel), labeldate, TRUE, TRUE, 5);

    null2label = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hboxlabel), null2label, TRUE, TRUE, 5);

    labelname = gtk_label_new("Name");
    gtk_label_set_xalign (labelname, 0.1);
    gtk_box_pack_start(GTK_BOX(hboxlabel), labelname, TRUE, TRUE, 5);
    
    labeltel = gtk_label_new("Telephone");
    gtk_label_set_xalign (labeltel, 0.3);
    gtk_box_pack_start(GTK_BOX(hboxlabel), labeltel, TRUE, TRUE, 5);

    labelnumpeople = gtk_label_new("Number of people");
    gtk_label_set_xalign (labelnumpeople, 0.3);
    gtk_box_pack_start(GTK_BOX(hboxlabel), labelnumpeople, TRUE, TRUE, 5);

    labelstatus = gtk_label_new("Status");
    gtk_label_set_xalign (labelstatus, 0.1);
    gtk_box_pack_start(GTK_BOX(hboxlabel), labelstatus, TRUE, TRUE, 5);

    nulllabel = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hboxlabel), nulllabel, TRUE, TRUE, 5);
    

    hboxentry = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox2), hboxentry, TRUE, TRUE, 0);

    hboxdate = gtk_hbox_new(TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hboxentry), hboxdate, FALSE, FALSE, 0);

    Entryday = gtk_entry_new();
    gtk_entry_set_max_length(Entryday,2);
    gtk_entry_set_placeholder_text(GTK_ENTRY(Entryday),"DD");
    gtk_box_pack_start(GTK_BOX(hboxdate), Entryday, FALSE,FALSE, 2);

    Entrymonth = gtk_entry_new();
    gtk_entry_set_max_length(Entrymonth,2);
    gtk_entry_set_placeholder_text(GTK_ENTRY(Entrymonth),"MM");
    gtk_box_pack_start(GTK_BOX(hboxdate), Entrymonth, FALSE, FALSE, 2);

    Entryyear = gtk_entry_new();
    gtk_entry_set_max_length(Entryyear,4);
    gtk_entry_set_placeholder_text(GTK_ENTRY(Entryyear),"YYYY");
    gtk_box_pack_start(GTK_BOX(hboxdate), Entryyear,FALSE, FALSE, 2);

    Entryname = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hboxentry), Entryname, TRUE, TRUE, 2);

    Entrytel = gtk_entry_new();
    gtk_entry_set_max_length(Entrytel,10);
    gtk_box_pack_start(GTK_BOX(hboxentry), Entrytel, TRUE, TRUE, 2);

    Entrynumpeople = gtk_entry_new();
    gtk_entry_set_max_length(Entrynumpeople,2);
    gtk_box_pack_start(GTK_BOX(hboxentry), Entrynumpeople, TRUE, TRUE, 2);

    Entrystatus = gtk_entry_new();
    gtk_entry_set_max_length(Entrystatus,3);
    gtk_box_pack_start(GTK_BOX(hboxentry), Entrystatus, TRUE, TRUE, 2);


    Bs2 = gtk_button_new_with_label("Search");
    gtk_widget_set_can_focus(Bs2, FALSE);
    gtk_widget_set_size_request(Bs2, 10, 10);
    gtk_box_pack_start(GTK_BOX(hboxentry), Bs2, TRUE, TRUE, 0);
    g_signal_connect(Bs2,"clicked",G_CALLBACK(search2button_callback),NULL );
  
    frame2 = gtk_frame_new("Data ");
    gtk_widget_set_size_request(frame2, 700, 700);

    eventbox2 = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(frame2), eventbox2);
    gtk_box_pack_start(GTK_BOX(vbox2), frame2, TRUE, TRUE, 0);

    scrwin2 = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_hexpand (scrwin2, TRUE);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrwin2), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_container_add (GTK_CONTAINER (eventbox2), scrwin2);

    listbox2 = gtk_list_box_new();
    gtk_list_box_set_selection_mode (GTK_LIST_BOX (listbox2), GTK_SELECTION_NONE);
    gtk_container_add (GTK_CONTAINER (scrwin2), listbox2);


    store2 = gtk_tree_store_new(
      COL_COUNTS, 
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,    
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING);

    GtkWidget *tree2 = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store2));
    gtk_container_add(GTK_CONTAINER(listbox2), tree2);
    
    GtkCellRenderer *renderer2;
    GtkTreeViewColumn *column2;

    renderer2 = gtk_cell_renderer_text_new();
    column2 = gtk_tree_view_column_new_with_attributes(
        "               TIME_LOGIN               ", 
        renderer2, 
        "text", COL_TIMLOG, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree2), column2);

    renderer2 = gtk_cell_renderer_text_new();
    column2 = gtk_tree_view_column_new_with_attributes(
        "QUEUE                                  ", 
        renderer2, 
        "text", COL_QUEUE, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree2), column2);

    renderer2 = gtk_cell_renderer_text_new();
    column2 = gtk_tree_view_column_new_with_attributes(
        "NAME                                  ", 
        renderer2, 
        "text", COL_NAME, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree2), column2);

    renderer2 = gtk_cell_renderer_text_new();
    column2 = gtk_tree_view_column_new_with_attributes(
        "NUMBER of PEOPLE                      ", 
        renderer2, 
        "text", COL_NUMBER, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree2), column2);

    renderer2 = gtk_cell_renderer_text_new();
    column2 = gtk_tree_view_column_new_with_attributes(
        "Telephone Number                      ", 
        renderer2, 
        "text", COL_TEL, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree2), column2);

    renderer2 = gtk_cell_renderer_text_new();
    column2 = gtk_tree_view_column_new_with_attributes(
        "STATUS                                ", 
        renderer2, 
        "text", COL_STATUS, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree2), column2);

    renderer2 = gtk_cell_renderer_text_new();
    column2 = gtk_tree_view_column_new_with_attributes(
        "          TIME STATUS          ", 
        renderer2, 
        "text", COL_TISTA, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree2), column2);


    hboxend = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox2), hboxend, TRUE, TRUE, 0);
    
    Bex2 = gtk_button_new_with_label("Exit");
    gtk_widget_set_can_focus(Bex2, FALSE);
    gtk_widget_set_size_request(Bex2, 300, 60);
    gtk_box_pack_start(GTK_BOX(hboxend), Bex2, TRUE, FALSE, 0);
    g_signal_connect(G_OBJECT(Bex2), "clicked", G_CALLBACK(btn_clicked), (gpointer) window);
    gtk_widget_show_all(window); 
    if(first_time)
    {
      refbutton_callback(NULL,user_data);
      first_time = 0;
    }
    gtk_main();
    
}
