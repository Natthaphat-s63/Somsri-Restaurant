//gcc -Wall backend.c -o backend -lpaho-mqtt3c -ljson-c -lmysqlclient `pkg-config --cflags gtk+-3.0` `pkg-config --libs gtk+-3.0` 

#include <json-c/json.h>
#include <json-c/json_inttypes.h>
#include <json-c/json_object.h>
#include <json-c/json_tokener.h>
#include <stdlib.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <unistd.h>
#include <string.h>
#include <mysql/mysql.h>
#include "MQTTClient.h"

#define NUM_JSON_ELEMENT 4
#define ADDRESS     "ws://your_ip:9001"
#define CLIENTID    "ExampleClientPub"
#define TOPIC       "test"
#define QOS         0
#define TIMEOUT     10000L

enum{
  COL_QUEUE, 
  COL_NAME,
  COL_NUMBER,
  COL_TEL,
  COL_TIME,
  COL_STATUS,
  COL_COUNTS
};


GtkApplication *app;

GtkTreeStore *store;
GtkTreeIter iter1;

GtkWidget *window;
GtkWidget *label;
GtkWidget *listbox;

gchar *text;
gint i;

static gboolean btn_clicked(GtkWidget *widget, gpointer parent);
//static GtkWidget *create_row(const gchar *text);

void subbutton_callback(GtkWidget *b, gpointer data );
void cancbutton_callback(GtkWidget *b, gpointer data);
void refbutton_callback(GtkWidget *b, gpointer data);

void activate( GtkApplication *app, gpointer user_data );
void view_selected(GtkTreeSelection *sel, gpointer data);
void finish_with_error(MYSQL *con);
//void view_dbclicked(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data);

char* obj [NUM_JSON_ELEMENT];
const char* key [NUM_JSON_ELEMENT] = {"Name","tel","num_people","time_arrive"};

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
		MYSQL *con = mysql_init(NULL);
    	if (con == NULL)
    	{	
        fprintf(stderr, "%s\n", mysql_error(con));
        return 0;
    	}
    	printf("Message arrived\n");
    	printf("     topic: %s\n", topicName);
    	
    	char* payloadptr = message->payload;
    	if (strcmp(payloadptr,"Disconnect unexpected")==0)return 0;
        json_object *json_obj = json_tokener_parse(payloadptr);         
		for(int i =0 ;i<NUM_JSON_ELEMENT;i++)
		{
			json_object *dummy;
			json_object_object_get_ex(json_obj, key[i], &dummy);
			obj[i]=json_object_get_string(dummy);
			printf("%s\n",obj[i]);
		}

		char query[256];
        sprintf(query,"INSERT INTO queue (Name,tel,num_people,time_arrive,queue,status) VALUES('%s','%s','%s','%s','1','Waiting')",obj[0],obj[1],obj[2],obj[3]);
		//printf("%s\n",query);
        //INSERT DATA
		if (mysql_real_connect(con,"localhost", "your_db_username", "your_db_password",
          "your_db", 0, NULL, 0) == NULL)
		{
    		finish_with_error(con);
		}
        if (mysql_query(con, query)) {
      	finish_with_error(con);
  		}
		mysql_close(con);
        MQTTClient_freeMessage(&message);
    	MQTTClient_free(topicName);
    	return 1;
}

void connlost(void *context, char *cause) {
    	printf("\nConnection lost\n");
    	printf("     cause: %s\n", cause);
}

MQTTClient client;
int main(int argc, char **argv){

	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	MQTTClient_create(&client,ADDRESS,CLIENTID,MQTTCLIENT_PERSISTENCE_NONE,NULL);
	MQTTClient_setCallbacks(client,NULL,connlost,msgarrvd,NULL);
  	int rc,ch;
	if((rc = MQTTClient_connect(client,&conn_opts))!=MQTTCLIENT_SUCCESS)
	{
		printf("Failed to connect, return code %d\n",rc);
		exit(-1);
	}
	printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
			"Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);

	app = gtk_application_new( NULL, G_APPLICATION_FLAGS_NONE );

	//g_timeout_add_seconds( 1 /*sec*/, G_SOURCE_FUNC(timeout), NULL );
  
	g_signal_connect( app, "activate", 
		G_CALLBACK(activate), NULL );
	// start the application main loop (blocking call)
	g_application_run( G_APPLICATION(app), argc, argv );
	// decrease the reference count to the object
	g_object_unref( app );
    	MQTTClient_disconnect(client, 10000);
    	MQTTClient_destroy(&client);
	return 0;

}

static gboolean btn_clicked(GtkWidget *widget, gpointer parent){
  GtkWidget *dialog;
  dialog = gtk_message_dialog_new(GTK_WINDOW(parent), 
              GTK_DIALOG_DESTROY_WITH_PARENT, 
              GTK_MESSAGE_QUESTION, 
              GTK_BUTTONS_YES_NO, 
              "Do you want to exit this program?");
  gtk_window_set_title(GTK_WINDOW(dialog), "Exit program");
  int result = gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
  if (result == GTK_RESPONSE_YES){
    gtk_main_quit();
    return FALSE;
  }else{
    return TRUE;
  }
}

/*static GtkWidget *create_row(const gchar *text){

  GtkWidget *row ,*box, *label1;

  row = gtk_list_box_row_new();
  box = gtk_event_box_new();
  gtk_container_add (GTK_CONTAINER (row), box);
  label1 = gtk_label_new(text);
  gtk_container_add (GTK_CONTAINER (box), label1);

  return row;
}*/

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
    const gchar *QUEUE, *NAME, *NUMBER, *TEL, *TIME, *STATUS, *p_Q;
    //const gint n = gtk_tree_model_iter_n_children (model, &iter);
    parent = iter;
    p_Q = "";
    if (gtk_tree_model_iter_parent(model, &parent, &iter)){
      //g_print("This iter have parent.\n");
      gtk_tree_model_get(model, &parent, COL_QUEUE, &p_Q, -1);
      //g_print("Parent author: %s\n", p_author);
    }
    
    gtk_tree_model_get(model, &iter, COL_QUEUE, &QUEUE, -1);
    gtk_tree_model_get(model, &iter, COL_NAME, &NAME, -1);
    gtk_tree_model_get(model, &iter, COL_NUMBER, &NUMBER, -1);
    gtk_tree_model_get(model, &iter, COL_TEL, &TEL, -1);
    gtk_tree_model_get(model, &iter, COL_TIME, &TIME, -1);
    gtk_tree_model_get(model, &iter, COL_STATUS, &STATUS, -1);
    
    sel_path = gtk_tree_model_get_path(model, &iter);
    const gchar *path_str = gtk_tree_path_to_string(sel_path);
    const gchar *iter_str = gtk_tree_model_get_string_from_iter(model, &iter);

    //g_print("%s:\n", iter_str);

    g_print("\tQueue: %s, Name: %s, Number of People: %s\n", QUEUE, NAME, NUMBER);
  }
}

void cancbutton_callback(GtkWidget *b1, gpointer data){
  gtk_tree_store_clear(store);
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
  if (mysql_real_connect(con, "localhost", "your_db_username", "your_db_password",
          "your_db", 0, NULL, 0) == NULL)
  {
      finish_with_error(con);
  }
  if (mysql_query(con, "SELECT Name,tel,num_people,time_arrive,queue,status FROM queue"))
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

        gtk_tree_store_append(store, &iter1, NULL);
        gtk_tree_store_set(store, &iter1, 
        COL_NAME, row[0], 
        COL_QUEUE, row[4], 
        COL_NUMBER, row[2],
        COL_TEL,row[1],
        COL_TIME,row[3],
        COL_STATUS,row[5], -1);
  }

  mysql_free_result(result);
  mysql_close(con);

}



void subbutton_callback(GtkWidget *b1, gpointer data){

    /*g_object_ref(label);
    label = gtk_label_new("Somsri Pochana");
    gtk_list_box_insert (GTK_LIST_BOX (listbox), label, -1);
    gtk_widget_show(label);*/
    gtk_tree_store_append(store, &iter1, NULL);
    gtk_tree_store_set(store, &iter1, 
        COL_NAME, "SomSri PoonSuk", 
        COL_QUEUE, "1", //int 
        COL_NUMBER, "3", //int
        COL_TEL,"0999989919",
        COL_TIME,"12.00", //edit to time
        COL_STATUS,"Super Hungry", -1);

}

void finish_with_error(MYSQL *con)
{
  fprintf(stderr, "%s\n", mysql_error(con));
  mysql_close(con);
  exit(1);
}

void activate( GtkApplication *app, gpointer user_data ){

    GtkWidget *scrwin, *frame, *vbox, *hbox, *eventbox;
    GtkWidget *Bob /*OpenBroker Button*/, *Bch /*Check Button*/ ,
              *Brf /*Refresh Button*/, *Bcc /*Cancle Button*/, *Bex /*Exit Button*/;

    //gtk_init(&argc, &argv);
    MQTTClient_subscribe(client, TOPIC, QOS);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Somsri Restaurant");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    //gtk_window_fullscreen(window);
    //g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(btn_clicked), (gpointer) window);

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(window), hbox);

    frame = gtk_frame_new("Data");
    gtk_widget_set_size_request(frame, 700, 700);

    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(frame), eventbox);
    gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE,TRUE,0);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

    scrwin = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_hexpand (scrwin, TRUE);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrwin), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_container_add (GTK_CONTAINER (eventbox), scrwin);


    listbox = gtk_list_box_new();
    gtk_list_box_set_selection_mode (GTK_LIST_BOX (listbox), GTK_SELECTION_NONE);
    gtk_container_add (GTK_CONTAINER (scrwin), listbox);


    /* 2. Create store */
    store = gtk_tree_store_new(
      COL_COUNTS,
      G_TYPE_STRING, 
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,    
      G_TYPE_STRING);

    /* 3. Create iter */

    gtk_tree_store_append(store, &iter1, NULL);
    gtk_tree_store_set(store, &iter1, 
        COL_NAME, "SomSri PoonSuk", 
        COL_QUEUE, "1", 
        COL_NUMBER, "3",
        COL_TEL,"0999989919",
        COL_TIME,"12.00",
        COL_STATUS,"Super Hungry", -1);
    
    gtk_tree_store_append(store, &iter1, NULL);
    gtk_tree_store_set(store, &iter1, 
        COL_NAME, "Somsak Chancha", 
        COL_QUEUE, "2", 
        COL_NUMBER, "99",
        COL_TEL,"0991125444",
        COL_TIME,"11.12",
        COL_STATUS,"VVIP", -1);

    /* 5. Creaete tree to contain store and keep tree widget to win */
    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_container_add(GTK_CONTAINER(listbox), tree);
    
    /* 6. Renderer data to view */
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "QUEUE", 
        renderer, 
        "text", COL_QUEUE, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

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
        "Time booking", 
        renderer, 
        "text", COL_TIME, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "STATUS", 
        renderer, 
        "text", COL_STATUS, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
    
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    gtk_tree_selection_set_mode(GTK_TREE_SELECTION(sel), GTK_SELECTION_SINGLE);

    //g_signal_connect(G_OBJECT(tree), "row-activated", G_CALLBACK(view_dbclicked), NULL);
    g_signal_connect(G_OBJECT(sel), "changed", G_CALLBACK(view_selected), NULL);

    Bob = gtk_button_new_with_label("Open Broker");
    gtk_widget_set_can_focus(Bob, FALSE);
    gtk_widget_set_size_request(Bob, 64, 64);
    gtk_box_pack_start(GTK_BOX(vbox), Bob, TRUE, TRUE, 0);
    
    Bch = gtk_button_new_with_label("Inspect Data");
    gtk_widget_set_can_focus(Bch, FALSE);
    gtk_widget_set_size_request(Bch, 64, 64);
    gtk_box_pack_start(GTK_BOX(vbox), Bch, TRUE, TRUE, 0);
    g_signal_connect(Bch,"clicked",G_CALLBACK(subbutton_callback),NULL);

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

    gtk_widget_show_all(window);
    gtk_main();
}
