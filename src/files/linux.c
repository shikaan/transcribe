// UNTESTED
#include "../types.h"
#include <gtk/gtk.h>
#include <stdbool.h>
#include <stdio.h>

result_t files_open(char *filepath, size_t maxlen) {
  static bool opening = false;
  if (opening)
    return ERR_FILES_CANCEL;
  opening = true;

  gtk_init(NULL, NULL);

  GtkWidget *dialog = gtk_file_chooser_dialog_new(
      "Open File", NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
      "_Cancel", GTK_RESPONSE_CANCEL,
      "_Open", GTK_RESPONSE_ACCEPT,
      NULL);

  GtkFileFilter *filter = gtk_file_filter_new();
  gtk_file_filter_set_name(filter, "Audio Files");
  gtk_file_filter_add_pattern(filter, "*.mp3");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

  result_t result = ERR_FILES_CANCEL;
  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    char *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    snprintf(filepath, maxlen, "%s", path);
    g_free(path);
    result = OK;
  }

  gtk_widget_destroy(dialog);
  opening = false;
  return result;
}
