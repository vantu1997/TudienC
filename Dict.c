#include<gtk/gtk.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include"jrb.h"
#include"btree.h"
#include"soundex.h"

GtkWidget *window;

GtkWidget *textView;
GtkWidget *textSearch;
GtkListStore *list;
GtkWidget *labelTitle;
GtkWidget *labelSoundexSuggest;

BTA *tree = NULL;

const char CONST_LABEL[] = "<span color=\"red\">*<i>Những từ có cùng âm sắc:</i></span>";
const int LINE_TYPE = 1;
const int NLINE_TYPE = 2;


int commond_char( char * str1, char * str2, int start) { //Tra ve vi tri str1 khac str2 bat dau tu start
	int i;
	int slen1 = strlen(str1);
	int slen2 = strlen(str2);
	int slen  = (slen1 < slen2) ? slen1 : slen2;
	for ( i = start; i < slen; i++)
		if (str1[i] != str2[i])
			return i;
	return i;
}

int prefix(const char * big, const char * small) { //kiem tra big co chua small hay ko (small la chuoi dau cua big)
	int small_len = strlen(small);
	int big_len = strlen(big);
	int i;
	if (big_len < small_len)
		return 0;
	for (i = 0; i < small_len; i++)
		if (big[i] != small[i])
			return 0;
	return 1;
}

int insert_insoundexlist(char * soundexlist , char * newword,  char * word, char * soundexWord, int type) { 
	//Them newword vao soundexlist neu thoa man dieu kien
	char tempSoundex[5]; //Luu ma soundex cua newword
	soundex(newword,tempSoundex);
	if (strcmp(soundexWord,tempSoundex) == 0) {
		if (strcmp(newword, word) != 0) {
			strcat(soundexlist, newword);
			if(type == NLINE_TYPE) strcat(soundexlist, "\n");
			else if(type == LINE_TYPE) strcat(soundexlist, ", ");
			return 1;
		}
	}
	else
		return 0;
}

void set_textView_text(GtkWidget *tView, char * text) { //Set text cho textView
	GtkTextBuffer *buffer; //Lưu trữ văn bản được phân bổ để hiển thị trong GtkTextView
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tView));
	if (buffer == NULL) {
		printf("Get buffer fail!");
		buffer = gtk_text_buffer_new(NULL); // NULL -> tao 1 table moi
	}
	gtk_text_buffer_set_text(buffer, text, -1); //Xoa noi dung hien tai va them 'text' vao buffer (If len is -1, text must be nul-terminated)
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(tView), buffer); //Dat buffer lam bo đem duoc hien thi boi textView
}

void jrb_to_list(JRB nextWordArray, int number) { // Lay ngau nhien 9 tu trong nextWordArray add vao list_store
	GtkTreeIter Iter;
	JRB tmp;
	int sochia = number / 9;
	int max = 8;
	if (sochia == 0) sochia = 1;
	jrb_traverse(tmp, nextWordArray) {
		if ((number--) % sochia == 0)  {
			gtk_list_store_append(list, &Iter);
			gtk_list_store_set(list, &Iter, 0, jval_s(tmp->key), -1 );
			// gtk_list_store_set(list, &Iter, 0, "", -1 );

			if (max-- < 1)
				return;
		}
	}
}

void suggest(char * word, gboolean Tab_pressed) {
	char nextword[100], prevword[100]; 
	int i, NumOfCommondChar, minNumOfCommondChar = 1000;
	GtkTreeIter Iter;
	JRB tmp;
	JRB nextWordArray = make_jrb(); //Luu nhung tu bat dau la 'word'
	BTint value;
	int existed = 0;
	strcpy(nextword, word);
	int wordlen = strlen(word);
	gtk_list_store_clear(list);

	if (bfndky(tree, word, &value) ==  0) { //tim 'word'
		existed = 1;
		gtk_list_store_append(list, &Iter);
		gtk_list_store_set(list, &Iter, 0, nextword, -1 ); 
	}
	if (!existed)
		btins(tree, nextword, "", 1); //chen them tu vaao jrb de tim kiem

	for (i = 0; i < 1945; i++) {
		bnxtky(tree, nextword, &value);  // duyet key tiep theo
		if (prefix(nextword, word)) {
			jrb_insert_str(nextWordArray, strdup(nextword), JNULL);
		}
		else break;
	}

	if (!existed && Tab_pressed) {
		if (jrb_empty(nextWordArray)) {
			char soundexlist[1000] = "Ý bạn là:\n";
			char soundexWord[5];
			strcpy(nextword, word);
			strcpy(prevword, word);
			soundex(word,soundexWord);
			printf("%s\n",soundexWord);
			int max = 5;
			bfndky(tree,word,&value);
			for (i = 10000; (i > 0 ) && max; i--) {
				if (bprvky(tree , prevword, &value) == 0) // duyet key truoc
					if (insert_insoundexlist(soundexlist, prevword, word, soundexWord, NLINE_TYPE))
						max--;
			}
			max += 5;
			bfndky(tree,word,&value);
			for (i = 10000; (i > 0 ) && max; i--) {
				if (bnxtky(tree, nextword, &value) == 0){ //duyet key sau
					if (insert_insoundexlist(soundexlist, nextword, word, soundexWord, NLINE_TYPE))
						max--;
				}
			}
			if(max!=10) set_textView_text(textView,soundexlist);
			else set_textView_text(textView,"Không có từ liên quan trong từ điển");
		}
		else {
			strcpy(nextword, jval_s(jrb_first(nextWordArray)->key)); //key la tu ngan nhat chua word vi nextWordArray tang dan
			jrb_traverse(tmp, nextWordArray) { //Duyet tree de tim vi tri ma tat ca cac key deu giong nhau, luu vi tri vao minNumCommondChar
				NumOfCommondChar = commond_char(nextword, jval_s(tmp->key), wordlen);
				if (minNumOfCommondChar > NumOfCommondChar)
					minNumOfCommondChar = NumOfCommondChar;
			}

			if ((minNumOfCommondChar  != 1000) && (minNumOfCommondChar > wordlen)) {
				nextword[NumOfCommondChar] = '\0';
				gtk_entry_set_text(GTK_ENTRY(textSearch), nextword);
				gtk_editable_set_position(GTK_EDITABLE(textSearch), -1); // -1 -> Chuyen con tro ve cuoi tu
			}
		}
		
	}
	else jrb_to_list(nextWordArray, i);

	if (!existed) btdel(tree, word); //Xoa 'word' vua them khi !existed
	jrb_free_tree(nextWordArray);
}


void on_key_down(GtkWidget * entry, GdkEvent * event, gpointer No_need) {
	GdkEventKey *keyEvent = (GdkEventKey *)event;
	char word[50];
	int len;
	strcpy(word, gtk_entry_get_text(GTK_ENTRY(textSearch)));
	if (keyEvent->keyval == GDK_KEY_Tab) {
		suggest(word,  TRUE);
	}
	else {
		if (keyEvent->keyval != GDK_KEY_BackSpace) {
			len = strlen(word);
			word[len] = keyEvent->keyval;
			word[len + 1] = '\0';
		}
		else {
			len = strlen(word);
			word[len - 1] = '\0';
		}
		suggest(word, FALSE);
	}
}


void find_in_dict(GtkWidget * widget, gpointer No_need) {
	const int MAX_LABEL_CHAR = 75; //Gioi han ki tu de label ko vuot qua H_SIZE_CONTAINER
	int rsize;
	char key[50];
	char mean[5000];
	strcpy(key, gtk_entry_get_text(GTK_ENTRY(textSearch)));
	if(strlen(key)==0){
		show_message(window,GTK_MESSAGE_WARNING,"Cảnh báo","Xin hãy nhập từ cần tìm kiếm");
		return;
	}
	if (btsel(tree, key, mean, 5000, &rsize)!=0)
		set_textView_text(textView,"\nRất tiếc, từ này hiện không có trong từ điển."
		                  "\n\nGợi ý:\t-Nhấn tab để tìm từ gần giống từ vừa nhập!"
		                  "\n\t\t-Hoặc nhấn nút \"Thêm/Sửa\", để bổ sung vào từ điển.");
	else set_textView_text(textView,mean);

	char nextword[100], prevword[100]; 
	char soundexlist[500] = "";
	char soundexWord[50];
	strcpy(nextword, key);
	strcpy(prevword, key);
	soundex(key,soundexWord);
	BTint value;
	int i, max = 3;
	bfndky(tree,key,&value);
	for (i = 10000; (i > 0 ) && max; i--) {
		if (bprvky(tree , prevword, &value) == 0){ // duyet key truoc
			if((strlen(prevword)+strlen(soundexlist))>MAX_LABEL_CHAR) continue;
			else if (insert_insoundexlist(soundexlist, prevword, key, soundexWord,LINE_TYPE)) max--;
		}
	}
	max +=3;
	bfndky(tree,key,&value);
	for (i = 10000; (i > 0 ) && max; i--) {
		if (bnxtky(tree, nextword, &value) == 0){
			if((strlen(prevword)+strlen(soundexlist))>MAX_LABEL_CHAR){
				max--;
				continue;
			}
			else if (insert_insoundexlist(soundexlist, nextword, key, soundexWord,LINE_TYPE)) max--;
		}
	}

	int len = strlen(soundexlist);
	soundexlist[len-2]='\n';
	soundexlist[len-1]='\0';
	gtk_label_set_markup(GTK_LABEL(labelTitle),CONST_LABEL); //Set text co dinh dang cho label
	if(max != 6){
		gtk_label_set_text(GTK_LABEL(labelSoundexSuggest),soundexlist);
	}
	else gtk_label_set_text(GTK_LABEL(labelSoundexSuggest),"Không có");
}

void clean_all(GtkWidget * widget, gpointer No_need){ //Lam moi cac thanh phan hien thi tren window
	gtk_entry_set_text(GTK_ENTRY(textSearch),"");
	set_textView_text(textView,"");
	gtk_label_set_text(GTK_LABEL(labelTitle),"");
	gtk_label_set_text(GTK_LABEL(labelSoundexSuggest),"");
	gtk_widget_grab_focus(textSearch);
}

void show_message(GtkWidget * parent , GtkMessageType type,  char * mms, char * content)
{
	GtkWidget *mdialog;
	mdialog = gtk_message_dialog_new(GTK_WINDOW(parent),
	                                 GTK_DIALOG_DESTROY_WITH_PARENT,
	                                 type,
	                                 GTK_BUTTONS_OK,
	                                 "%s", mms);
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(mdialog), "%s",  content); //Hien thi noi dung theo dinh dang
	gtk_dialog_run(GTK_DIALOG(mdialog));
	gtk_widget_destroy(mdialog);
}

char* get_text_from_textView(GtkWidget *tView){
	GtkTextIter st_iter; //Text buffer iterator
	GtkTextIter ed_iter;
	gtk_text_buffer_get_start_iter (gtk_text_view_get_buffer(GTK_TEXT_VIEW(tView)), &st_iter);//khoi tao st_iter voi vi tri dau tien trong bo dem
	gtk_text_buffer_get_end_iter (gtk_text_view_get_buffer(GTK_TEXT_VIEW(tView)), &ed_iter);//khoi tao ed_iter voi vi tri cuoi trong bo dem
	char * mean =  gtk_text_buffer_get_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(tView)), &st_iter, &ed_iter, FALSE);//Tra ve van ban trong pham vi [st_iter, ed_iter]
	                                                                                                         //FALSE -> Khong bao gom van ban chua dc hien thi
	return mean;
}

void check_word_add (GtkWidget *widget, GdkEvent  *event, gpointer data_array){ //Kiem tra tu trong inputText co trong tu dien hay ko
	                                                                            //Va thay doi label, button cho phu hop
	GtkWidget* checkLabel = ((GtkWidget**)data_array)[0];
	GtkWidget* inputText = ((GtkWidget**)data_array)[1];
	GtkWidget* meanText = ((GtkWidget**)data_array)[2];
	GtkWidget* OkButton = ((GtkWidget**)data_array)[3];
	char word[100];
	char mean[5000];
	int rsize;
	strcpy(word,gtk_entry_get_text(GTK_ENTRY(inputText)));
	if(word[0]=='\0') return;
	if(btsel(tree,word,mean,5000,&rsize)==0){
		gtk_label_set_markup(GTK_LABEL((GtkWidget*)checkLabel),"<span color=\"red\">Existed</span>");
		set_textView_text((GtkWidget*)meanText,mean);
		gtk_button_set_label(GTK_BUTTON(OkButton),"Sửa");
	}
	else{
		gtk_label_set_markup(GTK_LABEL((GtkWidget*)checkLabel),"<span color=\"green\">Not exist</span>");
		set_textView_text((GtkWidget*)meanText,"");
		gtk_button_set_label(GTK_BUTTON(OkButton),"Thêm");
	}
}



void click_add(GtkWidget *widget, gpointer data_array){ //Hien thi cac thong bao,... khi click vao button add
	GtkWidget* checkLabel = ((GtkWidget**)data_array)[0];
	GtkWidget* inputText = ((GtkWidget**)data_array)[1];
	GtkWidget* meanText = ((GtkWidget**)data_array)[2];
	char* word = gtk_entry_get_text(GTK_ENTRY(inputText));
	char* mean = get_text_from_textView(meanText);
	if (word[0] == '\0' || mean[0] == '\0'){
		show_message(((GtkWidget**)data_array)[4], GTK_MESSAGE_WARNING, "Cảnh báo!", "Không được bỏ trống phần nào.");
		return;
	}

	if(strcmp(gtk_label_get_text(GTK_LABEL(checkLabel)),"Existed")==0){
		if(btupd(tree,word,mean,strlen(mean)+1)==0){
			show_message(((GtkWidget**)data_array)[4],GTK_MESSAGE_INFO,"Thành công!","Đã cập nhật lại nghĩa của từ trong từ điển.");
		}
		else{			
			show_message(((GtkWidget**)data_array)[4],GTK_MESSAGE_ERROR,"Xảy ra lỗi!","Không thể cập nhật");
		}
	}
	else{
		if(btins(tree,word,mean,strlen(mean)+1)==0){
			show_message(((GtkWidget**)data_array)[4],GTK_MESSAGE_INFO,"Thành công!","Đã thêm từ vào từ điển");
		}
		else{
			show_message(((GtkWidget**)data_array)[4],GTK_MESSAGE_ERROR,"Xảy ra lỗi!","Không thể thêm từ");
		}
	}

	check_word_add(NULL,NULL,data_array);

}

void show_add_dialog(GtkWidget *widget,gpointer no_need){
	const int H_SIZE_DIALOG = 400;
	GtkWidget *adddialog;
	adddialog = gtk_dialog_new_with_buttons("Thêm/Sửa từ",GTK_WINDOW(window),GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,NULL);
	GtkWidget* dialog_ground = gtk_fixed_new();
	GtkWidget* tframe = gtk_frame_new("Từ vựng:");
	GtkWidget* bframe = gtk_frame_new("Nghĩa:");	
	GtkWidget* inputBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkWidget* buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkWidget* OkButton =  gtk_button_new_with_label("Thêm/Sửa");
	GtkWidget* CancelButton = gtk_button_new_with_label("Hủy");
	GtkWidget* inputText = gtk_entry_new();
	GtkWidget* checkLabel = gtk_label_new("...");
	GtkWidget* meanText = gtk_text_view_new();
	GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);

/* tframe*/
	gtk_widget_set_size_request(tframe,H_SIZE_DIALOG,50);
	gtk_container_add(GTK_CONTAINER(tframe),inputBox);
	gtk_widget_set_size_request(inputText,320,20);
	gtk_box_pack_start(GTK_BOX(inputBox),inputText,FALSE,FALSE,2);
	gtk_box_pack_start(GTK_BOX(inputBox),checkLabel,FALSE,FALSE,2);
	gtk_fixed_put(GTK_FIXED(dialog_ground),tframe,0,0);
/*end tframe*/

/* bframe */
	gtk_widget_set_size_request(bframe,H_SIZE_DIALOG,200);
	gtk_container_add(GTK_CONTAINER(scroll),meanText);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(meanText), GTK_WRAP_WORD_CHAR);//Chong tran be ngang
	gtk_container_add(GTK_CONTAINER(bframe),scroll);
	gtk_fixed_put(GTK_FIXED(dialog_ground),bframe,0,60);
/*end bframe */

/* buttonBox */
	gtk_widget_set_size_request(buttonBox,H_SIZE_DIALOG,50);
	gtk_box_pack_start(GTK_BOX(buttonBox),OkButton,TRUE,TRUE,2);
	gtk_widget_set_margin_top(OkButton,10);
	gtk_box_pack_start(GTK_BOX(buttonBox),CancelButton,TRUE,TRUE,2);
	gtk_widget_set_margin_top(CancelButton,10);
	gtk_box_set_homogeneous(GTK_BOX(buttonBox),TRUE); //Chia deu cho cac button vi label cua button co the lam thay doi size cua button
	gtk_fixed_put(GTK_FIXED(dialog_ground),buttonBox,0,260);
/*End buttonBox*/

	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(adddialog))), dialog_ground,  TRUE, TRUE, 5);
	//mac dinh dialog chi co 1 vung chua mac dinh la box

	GtkWidget * data_array[5];
	data_array[0] = checkLabel;
	data_array[1] = inputText;
	data_array[2] = meanText;
	data_array[3] = OkButton;
	data_array[4] = adddialog;

	g_signal_connect(inputText,"focus-out-event",G_CALLBACK(check_word_add),data_array);
	g_signal_connect(OkButton,"clicked",G_CALLBACK(click_add),data_array);
	g_signal_connect_swapped(CancelButton,"clicked",G_CALLBACK(gtk_widget_destroy),adddialog);
	gtk_widget_show_all(adddialog);
	gtk_dialog_run(GTK_DIALOG(adddialog));
	gtk_widget_destroy(adddialog);
}

void check_word_del(GtkWidget *widget, GdkEvent  *event, gpointer data_array){ //Kiem tra tu trong inputText co trong tu dien hay ko
	                                                                           //Va thay doi label, button cho phu hop
	GtkWidget* inputText = ((GtkWidget**)data_array)[0];
	GtkWidget* meanText = ((GtkWidget**)data_array)[1];
	char word[100];
	char mean[5000];
	int rsize;
	strcpy(word,gtk_entry_get_text(GTK_ENTRY(inputText)));
	if(word[0]=='\0') return;
	if(btsel(tree,word,mean,5000,&rsize)==0){
		set_textView_text((GtkWidget*)meanText,mean);
	}
	else{
		set_textView_text((GtkWidget*)meanText,"*Từ này không có trong từ điển");
		
	}
}

void click_del(GtkWidget *widget,gpointer data_array){ //Hien thi cac thong bao,... khi click vao button del
	GtkWidget *inputText = ((GtkWidget**)data_array)[0];
	GtkWidget *meanText = ((GtkWidget**)data_array)[1];

	char *word = gtk_entry_get_text(GTK_ENTRY(inputText));

	if (word[0] == '\0'){
		show_message(((GtkWidget**)data_array)[2], GTK_MESSAGE_WARNING, "Cảnh báo!", "Xin hãy nhập từ muốn xóa");
		return;
	}
	BTint value;
	if(bfndky(tree,word,&value)!=0){
		show_message(((GtkWidget**)data_array)[2], GTK_MESSAGE_ERROR, "Xảy ra lỗi!", "Từ này không có trong từ điển");
		return;
	}

	GtkWidget *quesdialog;
	quesdialog = gtk_message_dialog_new(GTK_WINDOW(window),
	                                   GTK_DIALOG_DESTROY_WITH_PARENT,
	                                   GTK_MESSAGE_QUESTION,
	                                   GTK_BUTTONS_YES_NO,
	                                   "Xóa: \"%s\"?", word); //(parent,flags,message type,button type,message format)
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(quesdialog), "Bạn thực sự muốn xóa từ \"%s\" chứ?", word);
	//Hien thi noi dung dialog theo dinh dang

	int result = gtk_dialog_run(GTK_DIALOG(quesdialog));
	if (result == GTK_RESPONSE_YES){
		if(btdel(tree,word)==0){
			char anu[100] = "Đã xóa từ ";
			show_message(window, GTK_MESSAGE_INFO, "Thành công!", strcat(strcat(anu, word), " khỏi từ điển"));
		}
		else{
			show_message(window, GTK_MESSAGE_ERROR, "Xảy ra lỗi!", "Không thể xóa");
		}
		set_textView_text(meanText,"");
		gtk_entry_set_text(GTK_ENTRY(inputText),"");
	}
	gtk_widget_destroy(quesdialog);

}

void show_del_dialog(GtkWidget *widget,gpointer no_need){
	const int H_SIZE_DIALOG = 400;
	GtkWidget *deldialog;
	deldialog = gtk_dialog_new_with_buttons("Xóa từ",GTK_WINDOW(window),GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,NULL);
	GtkWidget* dialog_ground = gtk_fixed_new();
	GtkWidget* tframe = gtk_frame_new("Từ vựng:");
	GtkWidget* bframe = gtk_frame_new("Nghĩa:");	
	GtkWidget* inputBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkWidget* buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkWidget* OkButton =  gtk_button_new_with_label("Xóa");
	GtkWidget* CancelButton = gtk_button_new_with_label("Hủy");
	GtkWidget* inputText = gtk_entry_new();
	GtkWidget* checkButton = gtk_button_new_with_label("Kiểm tra");
	GtkWidget* meanText = gtk_text_view_new();
	GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);

/* tframe*/
	gtk_widget_set_size_request(tframe,H_SIZE_DIALOG,50);
	gtk_container_add(GTK_CONTAINER(tframe),inputBox);
	gtk_widget_set_size_request(inputText,320,20);
	gtk_box_pack_start(GTK_BOX(inputBox),inputText,FALSE,FALSE,2);
	gtk_widget_grab_focus(inputText);

	gtk_box_pack_start(GTK_BOX(inputBox),checkButton,FALSE,FALSE,2);
	gtk_widget_set_margin_bottom(checkButton,2);
	gtk_fixed_put(GTK_FIXED(dialog_ground),tframe,0,0);
/*end tframe*/

/* bframe */
	gtk_widget_set_size_request(bframe,H_SIZE_DIALOG,200);
	gtk_container_add(GTK_CONTAINER(scroll),meanText);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(meanText),FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(meanText), GTK_WRAP_WORD_CHAR);//Chong tran be ngang
	gtk_container_add(GTK_CONTAINER(bframe),scroll);
	gtk_fixed_put(GTK_FIXED(dialog_ground),bframe,0,60);
/*end bframe */

/* buttonBox */
	gtk_widget_set_size_request(buttonBox,H_SIZE_DIALOG,50);
	gtk_box_pack_start(GTK_BOX(buttonBox),OkButton,TRUE,TRUE,2);
	gtk_widget_set_margin_top(OkButton,10);
	gtk_box_pack_start(GTK_BOX(buttonBox),CancelButton,TRUE,TRUE,2);
	gtk_widget_set_margin_top(CancelButton,10);
	gtk_box_set_homogeneous(GTK_BOX(buttonBox),TRUE); //Chia deu cho cac button
	gtk_fixed_put(GTK_FIXED(dialog_ground),buttonBox,0,260);
/*End buttonBox*/

	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(deldialog))), dialog_ground,  TRUE, TRUE, 5);

	// GdkWindow * w = gtk_widget_get_parent_window (inputText);
	// gdk_window_set_events ( w, GDK_FOCUS_CHANGE_MASK);

	GtkWidget * data_array[3];
	data_array[0] = inputText;
	data_array[1] = meanText;
	data_array[2] = deldialog;

	g_signal_connect(inputText,"focus-out-event",G_CALLBACK(check_word_del),data_array);
	g_signal_connect(OkButton,"clicked",G_CALLBACK(click_del),data_array);
	g_signal_connect_swapped(CancelButton,"clicked",G_CALLBACK(gtk_widget_destroy),deldialog);
	gtk_widget_show_all(deldialog);
	gtk_dialog_run(GTK_DIALOG(deldialog));
	gtk_widget_destroy(deldialog);
}

gboolean func(GtkEntryCompletion *completion, const gchar *key, GtkTreeIter *iter, gpointer user_data) {
	return TRUE;
}

int main(int argc, char **argv){
	tree = btopn("AnhViet.dat",0,1);

	const int H_SIZE_CONTAINER = 600;
	GtkWidget *container;
	GtkWidget *frameBox;
	GtkWidget *inputBox;
	GtkWidget *outputBox;
	GtkWidget *searchButton;
	GtkWidget *scrolling;
	GtkEntryCompletion *comple;
	GtkWidget *toolbar;
	

	GtkWidget *addButton;
	GtkWidget *delButton;
	GtkWidget *clearButton;

	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER);
	gtk_window_set_title(GTK_WINDOW(window),"Dictionary");
	gtk_window_set_resizable(GTK_WINDOW(window),FALSE);

/* container */
	container = gtk_fixed_new();
	gtk_container_add(GTK_CONTAINER(window),container);
	gtk_widget_set_margin_left(container,5);
	gtk_widget_set_margin_top(container,5);
	gtk_widget_set_margin_right(container,5);
	gtk_widget_set_margin_bottom(container,10);
/* end container */

/* toolbar */
	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH);
	// GdkColor color;
	// gdk_color_parse ("#141418", &color);
	// gtk_widget_modify_bg ( GTK_WIDGET(toolbar), GTK_STATE_NORMAL, &color);

	gtk_widget_set_size_request(toolbar,H_SIZE_CONTAINER,40);
	gtk_fixed_put(GTK_FIXED(container),toolbar,0,0);
	addButton = gtk_tool_button_new_from_stock(GTK_STOCK_ADD);
	gtk_tool_button_set_label(addButton,"Thêm/Sửa");
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),addButton,-1);
	delButton = gtk_tool_button_new_from_stock(GTK_STOCK_DELETE);
	gtk_tool_button_set_label(delButton,"Xóa từ");
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),delButton,-1);
	clearButton = gtk_tool_button_new_from_stock(GTK_STOCK_CLEAR);
	gtk_tool_button_set_label(clearButton,"Làm mới");
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),clearButton,-1);

/* end toolbar */


/* framebox */
	frameBox = gtk_frame_new("Tra từ");
	gtk_widget_set_size_request(frameBox,H_SIZE_CONTAINER,60);
	gtk_fixed_put(GTK_FIXED(container),frameBox,0,65);

	//inputbox
	inputBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
	gtk_container_add(GTK_CONTAINER(frameBox),inputBox);
	textSearch = gtk_search_entry_new();
	gtk_widget_set_size_request(textSearch,450,20);
	gtk_box_pack_start(GTK_BOX(inputBox),textSearch,TRUE,TRUE,5);
	gtk_widget_grab_focus(textSearch);
	searchButton = gtk_button_new_with_label("Tìm kiếm");
	gtk_widget_set_margin_bottom(searchButton,5);
	gtk_box_pack_start(GTK_BOX(inputBox),searchButton,TRUE,TRUE,5);

	comple = gtk_entry_completion_new();
	gtk_entry_completion_set_text_column(comple, 0); ///////////////////////////// list
	list = gtk_list_store_new(1, G_TYPE_STRING);

	gtk_entry_completion_set_model(comple, GTK_TREE_MODEL(list));
	gtk_entry_completion_set_match_func (comple, (GtkEntryCompletionMatchFunc)func, NULL, NULL); //Tuy chon hien thi comple theo dieu kien ham func
                                                             //gpointer func_data = NULL, GDestroyNotify func_notify = NULL
	gtk_entry_set_completion(GTK_ENTRY(textSearch), comple);
	//end inputbox
	
/* end framebox */

/* outputbox */
	outputBox = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
	gtk_widget_set_size_request(outputBox,H_SIZE_CONTAINER,400);
	gtk_fixed_put(GTK_FIXED(container),outputBox,0,130);

	textView = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textView), GTK_WRAP_WORD_CHAR);//Chong tran be ngang
	gtk_text_view_set_editable(GTK_TEXT_VIEW(textView),FALSE);
	gtk_widget_set_size_request(textView,H_SIZE_CONTAINER,50);

	scrolling = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scrolling), textView);
	gtk_box_pack_start(GTK_BOX(outputBox),scrolling,TRUE,TRUE,5);
	
	
	labelTitle = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(outputBox),labelTitle,FALSE,TRUE,5);
	gtk_label_set_xalign(GTK_LABEL(labelTitle),0);
	labelSoundexSuggest = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(outputBox),labelSoundexSuggest,FALSE,TRUE,5);
	gtk_label_set_xalign(GTK_LABEL(labelSoundexSuggest),0); //can trai. mac dinh them vao box la center
	gtk_widget_set_size_request(labelSoundexSuggest,H_SIZE_CONTAINER,-1);
	gtk_label_set_line_wrap(GTK_LABEL(labelSoundexSuggest),FALSE);

/* end outputbox */
	
	gtk_widget_show_all(window);



	g_signal_connect(window, "destroy", G_CALLBACK (gtk_main_quit), NULL);//Ket thuc chuong trinh khi dong cua so chinh
	g_signal_connect(textSearch,"key-press-event",G_CALLBACK(on_key_down),NULL); //Xu ly tung ki tu
	g_signal_connect(textSearch,"activate",G_CALLBACK(find_in_dict),NULL); //Tim kiem tu trong tu dien
	g_signal_connect(searchButton,"clicked",G_CALLBACK(find_in_dict),NULL);
	g_signal_connect(addButton,"clicked",G_CALLBACK(show_add_dialog),NULL); //Hien thi hop thoai Them/Sua key
	g_signal_connect(delButton,"clicked",G_CALLBACK(show_del_dialog),NULL); //Hien thi hop thoai Xoa key
	g_signal_connect(clearButton,"clicked",G_CALLBACK(clean_all),NULL);


	gtk_main();
	btcls(tree);
	return 0;
}