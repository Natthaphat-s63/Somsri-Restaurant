//gcc -Wall backend2.c -o b2 -lpaho-mqtt3c -ljson-c -lmysqlclient -lpthread `pkg-config --cflags gtk+-3.0` `pkg-config --libs gtk+-3.0` 
//mosquitto -c /etc/mosquitto/conf.d/default.conf

#include <json-c/json.h>
#include <json-c/json_inttypes.h>
#include <json-c/json_object.h>
#include <json-c/json_tokener.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <unistd.h>
#include <string.h>
#include <mysql/mysql.h>
#include "MQTTClient.h"

#define NUM_JSON_ELEMENT 3
#define ADDRESS     "ws://localhost:9001"
#define CLIENTID    "ExampleClientPub"
#define TOPIC       "test"
#define QOS         0
#define TIMEOUT     10000L

enum{
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


GtkApplication *app;

GtkTreeStore *store, *store2;
GtkTreeIter iter1, iter2;

GtkWidget *window;
GtkWidget *notebook, *tab1, *tab2;
GtkWidget *Entryq, *Entrydate, *Entryname, *Entrynumpeople ,*Entrytel;
GtkWidget *listbox, *listbox2;

gchar *text;
gchar *selected;

int state;
char date[11];

unsigned int Queue=0;
MQTTClient client;
static gboolean btn_clicked(GtkWidget *widget, gpointer parent);
//static GtkWidget *create_row(const gchar *text);

void searchbutton_callback(GtkWidget *b, gpointer data);
void confirmbutton_callback(GtkWidget *b, gpointer data);
void subbutton_callback(GtkWidget *b, gpointer data );
void cancbutton_callback(GtkWidget *b, gpointer data);
void refbutton_callback(GtkWidget *b, gpointer data);
void search2button_callback(GtkWidget *b, gpointer data);

void activate( GtkApplication *app, gpointer user_data );
void view_selected(GtkTreeSelection *sel, gpointer data);
void finish_with_error(MYSQL *con)
{
  fprintf(stderr, "%s\n", mysql_error(con));
  mysql_close(con);
  exit(1);
}

//void view_dbclicked(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data);



int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    char* obj [NUM_JSON_ELEMENT];
    const char* key [NUM_JSON_ELEMENT] = {"Name","tel","num_people"};
		MYSQL *con = mysql_init(NULL);
    	if (con == NULL)
    	{	
        fprintf(stderr, "%s\n", mysql_error(con));
        return 0;
    	}
    	printf("Message arrived\n");
    	printf("     topic: %s\n", topicName);
    	
    	char* payloadptr = message->payload;
      printf("%s\n",payloadptr);
    	if (strcmp(payloadptr,"Disconnect unexpected")==0)return 0;
      json_object *json_obj = json_tokener_parse(payloadptr);         
		  for(int i =0 ;i<NUM_JSON_ELEMENT;i++)
		  {
			  json_object *dummy;
			  json_object_object_get_ex(json_obj, key[i], &dummy);
			  obj[i]=json_object_get_string(dummy);
			  printf("%s \n",obj[i]);
		  }
        //INSERT DATA
		  if (mysql_real_connect(con,"localhost", "root", "password","mydb", 0, NULL, 0) == NULL)
		  {
    		finish_with_error(con);
		  }
    
      if (mysql_query(con, "SELECT queue FROM queue ORDER BY id DESC"))
      {
      	   finish_with_error(con);
  		}
        
      MYSQL_RES *result = mysql_store_result(con);
      if (result == NULL)
      {
         finish_with_error(con);
      }
      //int num_fields = mysql_num_fields(result);
      MYSQL_ROW row;
      row = mysql_fetch_row(result); 
      int recent_queue=1; 
    
      if(row!=NULL)
      {
        recent_queue =atoi(row[0]);
        recent_queue++; // for next queue
      }
      mysql_free_result(result);
  
      char query[256];
      sprintf(query,"INSERT INTO queue (Name,tel,num_people,queue,status) VALUES('%s','%s','%s','%d','WFA')",obj[0],obj[1],obj[2],recent_queue);
      if (mysql_query(con, query))
      {
      	   finish_with_error(con);
  		}
		  mysql_close(con);

      if (state == 0){

        char recentq[10]; 
        sprintf(recentq,"%d",recent_queue);

        gtk_tree_store_append(store, &iter1, NULL);
          gtk_tree_store_set(store, &iter1,
          COL_ID,  "", 
          COL_NAME, obj[0], 
          COL_QUEUE, recentq, 
          COL_NUMBER, obj[2],
          COL_TEL,obj[1],
          COL_STATUS,"WFA",
          COL_TISTA ,"", -1);

      }
      
      MQTTClient_freeMessage(&message);
    	MQTTClient_free(topicName);
    	return 1;
}

void connlost(void *context, char *cause) {
    	printf("\nConnection lost\n");
    	printf("     cause: %s\n", cause);
}

int main(int argc, char **argv){

  time_t rawt = time(NULL);
  struct tm  *time = localtime(&rawt);
  sprintf(date,"%d-%02d-%02d",time->tm_year + 1900,time->tm_mon + 1,time->tm_mday);
  int r = system("x-terminal-emulator -e \"/home/hill/Desktop/backend/week2/open_broker.sh\"");
  MQTTClient_create(&client,ADDRESS,CLIENTID,MQTTCLIENT_PERSISTENCE_NONE,NULL);
	MQTTClient_setCallbacks(client,NULL,connlost,msgarrvd,NULL);
  usleep(100000);
  int rc;
  MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
  if((rc = MQTTClient_connect(client,&conn_opts))!=MQTTCLIENT_SUCCESS)
	{
		printf("Failed to connect, return code %d\n",rc);
		exit(-1);
	}
  
	
  app = gtk_application_new( NULL, G_APPLICATION_FLAGS_NONE );        //create application
	//g_timeout_add_seconds( 1 /*sec*/, G_SOURCE_FUNC(timeout), NULL );
	g_signal_connect( app, "activate", G_CALLBACK(activate), NULL );    
	// start the application main loop (blocking call)
  g_application_run( G_APPLICATION(app), argc, argv );
	// decrease the reference count to the object
	g_object_unref( app );
  MQTTClient_disconnect(client, 10000);
  MQTTClient_destroy(&client);
  r = system("/home/hill/Desktop/backend/week2/kill_broker.sh");
	return 0;
  

}

static gboolean btn_clicked(GtkWidget *widget, gpointer parent){
  GtkWidget *dialog;
  dialog = gtk_message_dialog_new(GTK_WINDOW(parent),     //create popup window
              GTK_DIALOG_DESTROY_WITH_PARENT, 
              GTK_MESSAGE_QUESTION, 
              GTK_BUTTONS_YES_NO, 
              "Do you want to exit this program?");
  gtk_window_set_title(GTK_WINDOW(dialog), "Exit program");
  int result = gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
  if (result == GTK_RESPONSE_YES){
    MQTTClient_disconnect(client, 10000);
    gtk_main_quit();
    return FALSE;

  }else{
    return TRUE;
  }
}

/*void view_dbclicked(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data){
  //GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
  GtkTreeModel *model = gtk_tree_view_get_model(treeview);
  GtkTreeIter iter;
  //GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
  
  
  if (gtk_tree_model_get_iter(model, &iter, path)){
    if (gtk_tree_view_row_expanded(treeview, path)){
      gtk_tree_view_collapse_row(treeview, path);
    }else{
      gtk_tree_view_expand_row(treeview, path, TRUE);
    }
    g_print("Double Clicked\n");
  }
}*/

void view_selected(GtkTreeSelection *sel, gpointer data){

  GtkTreeIter iter, parent;
  GtkTreePath *sel_path;
  GtkTreeModel *model;
  if (gtk_tree_selection_get_selected(sel, &model, &iter)){
    const gchar *QUEUE, *NAME, *NUMBER, *TEL, *STATUS, *ID, *TISTA;
    //const gint n = gtk_tree_model_iter_n_children (model, &iter);
    parent = iter;
    /*p_Q = "";
    if (gtk_tree_model_iter_parent(model, &parent, &iter)){
      //g_print("This iter have parent.\n");
      gtk_tree_model_get(model, &parent, COL_QUEUE, &p_Q, -1);
      //g_print("Parent author: %s\n", p_author);
    }*/

    gtk_tree_model_get(model, &iter, COL_ID, &ID, -1);        //Gets the value displayed for each column in the same row. 
    gtk_tree_model_get(model, &iter, COL_QUEUE, &QUEUE, -1);
    gtk_tree_model_get(model, &iter, COL_NAME, &NAME, -1);
    gtk_tree_model_get(model, &iter, COL_NUMBER, &NUMBER, -1);
    gtk_tree_model_get(model, &iter, COL_TEL, &TEL, -1);
    gtk_tree_model_get(model, &iter, COL_STATUS, &STATUS, -1);
    gtk_tree_model_get(model, &iter, COL_TISTA, &TISTA, -1);
    
    /*sel_path = gtk_tree_model_get_path(model, &iter);
    const gchar *path_str = gtk_tree_path_to_string(sel_path);*/
    //const gchar *iter_str = gtk_tree_model_get_string_from_iter(model, &iter);

    //g_print("%s:\n", iter_str);

    g_print("\tQueue: %s, Name: %s, Number of People: %s\n", QUEUE, NAME, NUMBER);
    //g_print("\tID : %s\n\n", ID);
    selected = ID;
    g_print("\tID : %s\n\n", selected);

  }
}

void searchbutton_callback(GtkWidget *b, gpointer data){

  const gchar *queue_id = gtk_entry_get_text(GTK_ENTRY(Entryq));    //Gets the value that is in the id entry.

  gtk_tree_store_clear(store);  //clear all liststore

  MYSQL *con = mysql_init(NULL);
  if (con == NULL)
  {
      fprintf(stderr, "mysql_init() failed\n");
      exit(1);
  }
  if (mysql_real_connect(con, "localhost", "root", "password",
          "mydb", 0, NULL, 0) == NULL)
  {
      finish_with_error(con);
  }

  if (mysql_query(con, "SELECT id,Name,time_login,tel,num_people,queue,status,time_status FROM queue"))
  {
      finish_with_error(con);
  }

  MYSQL_RES *result = mysql_store_result(con); //all data

  if (result == NULL)
  {
      finish_with_error(con);
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
      else{

        continue;
      }    
  }



}

void search2button_callback(GtkWidget *b, gpointer data){

  char query[256];
  int count = 0;

  char *name2 = gtk_entry_get_text(GTK_ENTRY(Entryname));
  char *tel2 = gtk_entry_get_text(GTK_ENTRY(Entrytel));
  char *numpeople2 = gtk_entry_get_text(GTK_ENTRY(Entrynumpeople));
  printf("%s %s %s \n",name2,tel2,numpeople2);

  gtk_tree_store_clear(store2);

  MYSQL *con = mysql_init(NULL);
  if (con == NULL)
  {
      fprintf(stderr, "mysql_init() failed\n");
      exit(1);
  }
  if (mysql_real_connect(con, "localhost", "root", "password",
          "mydb", 0, NULL, 0) == NULL)
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
    printf("%d",count);

    if (count == 3){
        sprintf(query,"SELECT  * FROM queue");
    }
    else if (count == 2)
    {
        sprintf(query,"SELECT  * FROM queue WHERE Name = '%s' OR tel = '%s' OR num_people = %d",name2,tel2,atoi(numpeople2));
    }
    else if (count == 1)
    {
        sprintf(query,"SELECT  * FROM queue WHERE (Name = '%s' AND tel = '%s') OR (Name = '%s' AND num_people = %d) OR (tel = '%s'  AND num_people = %d) OR (tel = '%s'  AND Name = '%s')",name2,tel2,name2,atoi(numpeople2),tel2,atoi(numpeople2),tel2,name2);
    }
     else if (count == 0)
    {
        sprintf(query,"SELECT  * FROM queue WHERE Name = '%s' AND tel = '%s' AND num_people = %d",name2,tel2,atoi(numpeople2));
    }

  if (mysql_query(con, query))
  {
      finish_with_error(con);
  }


  MYSQL_RES *result = mysql_store_result(con); //all data

  if (result == NULL)
  {
      finish_with_error(con);
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

  mysql_free_result(result);
  mysql_close(con);



}

void confirmbutton_callback(GtkWidget *b, gpointer data){

  MYSQL *con = mysql_init(NULL);
  
  if (con == NULL)
  {
      fprintf(stderr, "mysql_init() failed\n");
      exit(1);
  }
  if (mysql_real_connect(con, "localhost", "root", "password",
          "mydb", 0, NULL, 0) == NULL)
  {
      finish_with_error(con);
  }

  int sel = atoi(selected);
  char query[256];
      sprintf(query,"UPDATE queue SET status='DON' WHERE id = %d",sel);
      if (mysql_query(con, query))
      {
             finish_with_error(con);
      }

  //gtk_tree_store_clear(store);
  refbutton_callback(b,NULL);
}

void cancbutton_callback(GtkWidget *b1, gpointer data){

  MYSQL *con = mysql_init(NULL);
  
  if (con == NULL)
  {
      fprintf(stderr, "mysql_init() failed\n");
      exit(1);
  }
  if (mysql_real_connect(con, "localhost", "root", "password",
          "mydb", 0, NULL, 0) == NULL)
  {
      finish_with_error(con);
  }
  if (selected == NULL){
    return;
  }
  int sel = atoi(selected);
  char query[256];
      sprintf(query,"UPDATE queue SET status='CBA' WHERE id = %d",sel);
      if (mysql_query(con, query))
      {
             finish_with_error(con);
      }

  //gtk_tree_store_clear(store);
  refbutton_callback(b1,NULL);
  /*Apeend Dump Ja */
}


void refbutton_callback(GtkWidget *b1, gpointer data){

  

  gtk_tree_store_clear(store);

  MYSQL *con = mysql_init(NULL);
  if (con == NULL)
  {
      fprintf(stderr, "mysql_init() failed\n");
      exit(1);
  }
  if (mysql_real_connect(con, "localhost", "root", "password",
          "mydb", 0, NULL, 0) == NULL)
  {
      finish_with_error(con);
  }

  char query[256];
  sprintf(query,"SELECT * FROM queue WHERE DATE(time_login) = '%s' AND status = 'WFA'",date);
  if (mysql_query(con, query))finish_with_error(con);
  /*if (mysql_query(con, "SELECT id,Name,tel,num_people,queue,status,time_status FROM queue"))
  {
      finish_with_error(con);
  }*/



  MYSQL_RES *result = mysql_store_result(con); //all data

  if (result == NULL)
  {
      finish_with_error(con);
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

  mysql_free_result(result);
  mysql_close(con);

}

void activate( GtkApplication *app, gpointer user_data ){

  
    GtkWidget *scrwin, *frame, *vbox, *hbox, *eventbox, *vbox0, *vbox00, *hbox0, *labelid;
    GtkWidget *Brf /*Refresh Button*/, *Bcc /*Cancle Button*/, *Bex /*Exit Button*/,
              *Bs /*Search Button*/, *Bcf /*Confirm Button*/;

    //gtk_init(&argc, &argv);
    MQTTClient_subscribe(client, TOPIC, QOS);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);                                                 //create window
    gtk_window_set_title(GTK_WINDOW(window), "Somsri Restaurant");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    //gtk_window_fullscreen(window);
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(btn_clicked), (gpointer) window);    //If selected, press the destroy button on the top right.

    vbox00 = gtk_vbox_new (FALSE, 0);                                                             //Vertical box, if added later will be added below.
    gtk_container_add (GTK_CONTAINER (window), vbox00);                                           //Put this box in Windows

    notebook = gtk_notebook_new ();                                                               //create notebook
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);                              //set tap position (top)
    gtk_box_pack_start (GTK_BOX (vbox00), notebook, TRUE, TRUE, 0);                               //put this widgets in this box

    tab1 = gtk_label_new ("Tab1");
    gtk_widget_show (tab1);

    vbox0 = gtk_vbox_new(FALSE, 5);
    //gtk_container_add(GTK_CONTAINER(window), vbox0);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox0 , tab1);                             //add vbox0 in tab1

    hbox0 = gtk_hbox_new(FALSE, 5);                                                               //Horizontal box, if add widget, it will add right side.
    gtk_box_pack_start(GTK_BOX(vbox0), hbox0, TRUE, TRUE, 0);

    labelid = gtk_label_new("Enter Queue: ");
    gtk_box_pack_start(GTK_BOX(hbox0), labelid, TRUE, TRUE, 0);

    Entryq = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox0), Entryq, TRUE, TRUE, 0);

    Bs = gtk_button_new_with_label("Search");
    gtk_widget_set_can_focus(Bs, FALSE);                                                        //Specifies whether widget can own the input focus
    gtk_widget_set_size_request(Bs, 10, 10);
    gtk_box_pack_start(GTK_BOX(hbox0), Bs, TRUE, TRUE, 0);
    g_signal_connect(Bs,"clicked",G_CALLBACK(searchbutton_callback),NULL /*data callback*/);    //If a button is pressed, the callback function is activated.
    //g_signal_connect(Bs,"clicked",G_CALLBACK(refbutton_callback),NULL);

    hbox = gtk_hbox_new(FALSE, 5);
    //gtk_container_add(GTK_CONTAINER(window), hbox);
    gtk_box_pack_start(GTK_BOX(vbox0), hbox, TRUE, TRUE, 0);

    frame = gtk_frame_new("Data");
    gtk_widget_set_size_request(frame, 700, 700);

    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(frame), eventbox);
    gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE,TRUE,0);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

    scrwin = gtk_scrolled_window_new (NULL, NULL);  
    gtk_widget_set_hexpand (scrwin, TRUE);                            //Sets whether the widget would like any available extra horizontal space. When a user resizes a GtkWindow, widgets with expand=TRUE generally receive the extra space.
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrwin), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_container_add (GTK_CONTAINER (eventbox), scrwin);                                     //add scrollbar to eventbox 


    listbox = gtk_list_box_new();
    gtk_list_box_set_selection_mode (GTK_LIST_BOX (listbox), GTK_SELECTION_NONE);       //You can select a value in the listbox
    gtk_container_add (GTK_CONTAINER (scrwin), listbox);


    /* 2. Create store */
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

    /* 5. Creaete tree to contain store and keep tree widget to win */
    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_container_add(GTK_CONTAINER(listbox), tree);
    
    /* 6. Renderer data to view */
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    /*renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "ID", 
        renderer, 
        "text", COL_ID, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);*/

    renderer = gtk_cell_renderer_text_new();                                //widgets are used to display information within widgets
    column = gtk_tree_view_column_new_with_attributes(                      //create column
        "QUEUE", 
        renderer, 
        "text", COL_QUEUE, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);               //append the column

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "NAME", 
        renderer, 
        "text", COL_NAME, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "NUMBER of PEOPLE", 
        renderer, 
        "text", COL_NUMBER, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "Telephone Number", 
        renderer, 
        "text", COL_TEL, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "STATUS", 
        renderer, 
        "text", COL_STATUS, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "TIME STATUS", 
        renderer, 
        "text", COL_TISTA, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
    
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    gtk_tree_selection_set_mode(GTK_TREE_SELECTION(sel), GTK_SELECTION_SINGLE);

    //g_signal_connect(G_OBJECT(tree), "row-activated", G_CALLBACK(view_dbclicked), NULL);
    g_signal_connect(G_OBJECT(sel), "changed", G_CALLBACK(view_selected), NULL);

    Bcf = gtk_button_new_with_label("Confirm");
    gtk_widget_set_can_focus(Bcf, FALSE);
    gtk_widget_set_size_request(Bcf, 64, 64);
    gtk_box_pack_start(GTK_BOX(vbox), Bcf, TRUE, TRUE, 0);
    g_signal_connect(Bcf,"clicked",G_CALLBACK(confirmbutton_callback),NULL);

    Brf = gtk_button_new_with_label("Refresh");
    gtk_widget_set_can_focus(Brf, FALSE);
    gtk_widget_set_size_request(Brf, 64, 64);
    gtk_box_pack_start(GTK_BOX(vbox), Brf, TRUE, TRUE, 0);
    g_signal_connect(Brf,"clicked",G_CALLBACK(refbutton_callback),NULL);
    //g_signal_connect(b3, "clicked", G_CALLBACK(gtk_main_quit), NULL);
    
    Bcc = gtk_button_new_with_label("Cancel");
    gtk_widget_set_can_focus(Bcc, FALSE);
    gtk_widget_set_size_request(Bcc, 64, 64);
    gtk_box_pack_start(GTK_BOX(vbox), Bcc, TRUE, TRUE, 0);
    g_signal_connect(Bcc,"clicked",G_CALLBACK(cancbutton_callback),NULL);

    Bex = gtk_button_new_with_label("Exit");
    gtk_widget_set_can_focus(Bex, FALSE);
    gtk_widget_set_size_request(Bex, 64, 64);
    gtk_box_pack_start(GTK_BOX(vbox), Bex, TRUE, TRUE, 0);
    g_signal_connect(G_OBJECT(Bex), "clicked", G_CALLBACK(btn_clicked), (gpointer) window);



    //tab2

    GtkWidget *scrwin2, *frame2, *vbox2, *hboxlabel, *hboxentry, *eventbox2, *listbox2,
              *labeldate, *labelname, *labelnumpeople, *labeltel, *nulllabel;
    GtkWidget *Bs2 /*Search Button*/, *Bex2 /*Exit Button*/;

    
    tab2 = gtk_label_new ("Tab2");
    gtk_widget_show (tab2);

    vbox2 = gtk_vbox_new(FALSE, 5);
    //gtk_container_add(GTK_CONTAINER(window), vbox2);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox2 , tab2);

    hboxlabel = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox2), hboxlabel, TRUE, TRUE, 0);

    labelname = gtk_label_new("Name");
    gtk_box_pack_start(GTK_BOX(hboxlabel), labelname, TRUE, TRUE, 0);
    labeldate = gtk_label_new("Date");
    gtk_box_pack_start(GTK_BOX(hboxlabel), labeldate, TRUE, TRUE, 0);
    labeltel = gtk_label_new("Telephone");
    gtk_box_pack_start(GTK_BOX(hboxlabel), labeltel, TRUE, TRUE, 0);
    labelnumpeople = gtk_label_new("Number of people");
    gtk_box_pack_start(GTK_BOX(hboxlabel), labelnumpeople, TRUE, TRUE, 0);
    nulllabel = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hboxlabel), nulllabel, TRUE, TRUE, 0);

    hboxentry = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox2), hboxentry, TRUE, TRUE, 0);

    Entryname = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hboxentry), Entryname, TRUE, TRUE, 0);
    Entrydate = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hboxentry), Entrydate, TRUE, TRUE, 0);
    Entrytel = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hboxentry), Entrytel, TRUE, TRUE, 0);
    Entrynumpeople = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hboxentry), Entrynumpeople, TRUE, TRUE, 0);

    Bs2 = gtk_button_new_with_label("Search");
    gtk_widget_set_can_focus(Bs2, FALSE);
    gtk_widget_set_size_request(Bs2, 10, 10);
    gtk_box_pack_start(GTK_BOX(hboxentry), Bs2, TRUE, TRUE, 0);
     g_signal_connect(Bs2,"clicked",G_CALLBACK(search2button_callback),NULL /*data callback*/);
  
    frame2 = gtk_frame_new("Data");
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

    /* 5. Creaete tree to contain store and keep tree widget to win */
    GtkWidget *tree2 = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store2));
    gtk_container_add(GTK_CONTAINER(listbox2), tree2);
    
    /* 6. Renderer data to view */
    GtkCellRenderer *renderer2;
    GtkTreeViewColumn *column2;

    renderer2 = gtk_cell_renderer_text_new();
    column2 = gtk_tree_view_column_new_with_attributes(
        "TIME_LOGIN", 
        renderer, 
        "text", COL_TIMLOG, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree2), column2);

    renderer2 = gtk_cell_renderer_text_new();
    column2 = gtk_tree_view_column_new_with_attributes(
        "QUEUE", 
        renderer, 
        "text", COL_QUEUE, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree2), column2);

    renderer2 = gtk_cell_renderer_text_new();
    column2 = gtk_tree_view_column_new_with_attributes(
        "NAME", 
        renderer, 
        "text", COL_NAME, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree2), column2);

    renderer2 = gtk_cell_renderer_text_new();
    column2 = gtk_tree_view_column_new_with_attributes(
        "NUMBER of PEOPLE", 
        renderer, 
        "text", COL_NUMBER, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree2), column2);

    renderer2 = gtk_cell_renderer_text_new();
    column2 = gtk_tree_view_column_new_with_attributes(
        "Telephone Number", 
        renderer, 
        "text", COL_TEL, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree2), column2);

    renderer2 = gtk_cell_renderer_text_new();
    column2 = gtk_tree_view_column_new_with_attributes(
        "STATUS", 
        renderer, 
        "text", COL_STATUS, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree2), column2);

    renderer2 = gtk_cell_renderer_text_new();
    column2 = gtk_tree_view_column_new_with_attributes(
        "TIME STATUS", 
        renderer, 
        "text", COL_TISTA, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree2), column2);


    Bex2 = gtk_button_new_with_label("Exit");
    gtk_widget_set_can_focus(Bex2, FALSE);
    gtk_widget_set_size_request(Bex2, 64, 64);
    gtk_box_pack_start(GTK_BOX(vbox2), Bex2, TRUE, TRUE, 0);
    g_signal_connect(G_OBJECT(Bex2), "clicked", G_CALLBACK(btn_clicked), (gpointer) window);

    gtk_widget_show_all(window); 
    gtk_main();
    
}
