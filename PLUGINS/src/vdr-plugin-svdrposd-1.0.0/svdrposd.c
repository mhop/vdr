/*
 * svdrposd.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/plugin.h>
#include <vdr/tools.h>
#include <strings.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "status.h"

#define SVDRPOSD_BUFSIZE KILOBYTE(4)

static const char *VERSION        = "1.0.0";
static const char *DESCRIPTION    = "Publish OSD menu via SVDRP";

class cPluginSvdrpOsd : public cPlugin {
private:
  // Add any member variables or functions you may need here.
  cSvdrpOsdStatus * status;
  int length;
  int size;
  char *buffer;

  bool Append(const char *Fmt, ...);
  bool AppendItems(const char* Option);
  const char* Reply();
  void Reset();
public:
  cPluginSvdrpOsd(void);
  virtual ~cPluginSvdrpOsd();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return DESCRIPTION; }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual const char *MainMenuEntry(void) { return NULL; }
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  virtual bool Service(const char *Id, void *Data = NULL);
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
  };

cPluginSvdrpOsd::cPluginSvdrpOsd(void)
{
  status = NULL;
  length = size = 0;
  buffer = NULL;
}

cPluginSvdrpOsd::~cPluginSvdrpOsd()
{
  // Clean up after yourself!
  delete(status);
}

const char *cPluginSvdrpOsd::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginSvdrpOsd::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginSvdrpOsd::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
  status = new cSvdrpOsdStatus();
  return true;
}

bool cPluginSvdrpOsd::Start(void)
{
  // Start any background activities the plugin shall perform.
  return true;
}

void cPluginSvdrpOsd::Stop(void)
{
  // Stop any background activities the plugin shall perform.
}

void cPluginSvdrpOsd::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

cOsdObject *cPluginSvdrpOsd::MainMenuAction(void)
{
  return NULL;
}

cMenuSetupPage *cPluginSvdrpOsd::SetupMenu(void)
{
  return NULL;
}

bool cPluginSvdrpOsd::SetupParse(const char *Name, const char *Value)
{
  return false;
}

bool cPluginSvdrpOsd::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  return false;
}

const char **cPluginSvdrpOsd::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
  static const char *HelpPages[] = {
  "LSTO [items_per_page]\n"
        "    Print the OSD contents. Each line starts with a prefix denoting the\n"
        "    type of information which follows:\n"
        "      T: OSD title\n"
        "      C: column layout (the tab stops)\n"
        "      I: menu item\n"
        "      S: selected menu item\n"
        "      X: textblock\n"
        "      R/G/Y/B: color key help titles\n"
        "      M: status message\n"
        "    The optional parameter 'items_per_page' limits the amount of items\n"
        "    returned.\n",
  "OSDT\n"
        "    Return the current OSD title\n",
  "OSDI [items_per_page]\n"
        "    Return up to 'items_per_page' OSD entries. All items are returned\n"
        "    if the argument is missing or 0.\n"
        "    If applicable, the column layout is printed first, followed by\n"
        "    the actual entries. The layout part consists of one line per\n"
        "    column. Each line starts with the prefix C: followed by the width.\n"
        "    The prefix I: is used for all items except for the selected item\n"
        "    which is prefixed by S:.\n",
  "OSDX\n"
        "    Return the textblock which is currently shown on the OSD.\n",
  "OSDH\n"
        "    Return the current OSD color key help titles. Each color is\n"
        "    returned on a line of its own, prefixed by R:, G:, Y: or B:,\n"
        "    respectively. For unused help buttons only the prefix is returned.\n",
  "OSDM\n"
        "    Return the current OSD status message.\n",
  NULL
  };
  return status ? HelpPages : NULL;
}

bool cPluginSvdrpOsd::Append(const char *Fmt, ...)
{
  va_list ap;

  if (!buffer) {
	  length = 0;
	  size = SVDRPOSD_BUFSIZE;
	  buffer = (char *) malloc(sizeof(char) * size);
  }
  while (buffer) {
	  va_start(ap, Fmt);
	  int n = vsnprintf(buffer + length, size - length, Fmt, ap);
	  va_end(ap);

	  if (n < size - length) {
		  length += n;
		  return true;
	  }
	  // overflow: realloc and try again
	  size += SVDRPOSD_BUFSIZE;
	  char *tmp = (char *) realloc(buffer, sizeof(char) * size);
	  if (!tmp)
		  free(buffer);
	  buffer = tmp;
  }
  return false;
}

/**
 * Appends item list for OSDI and LSTO to buffer.
 */
bool cPluginSvdrpOsd::AppendItems(const char *Option)
{
  bool ok = true;
  int maxitems = (*Option) ? strtol(Option, NULL, 10) : -1;

  // columns
  for (int i = 0; ok && i < status->MaxTabs && status->tabs[i] != 0; i++)
	  ok = Append("C:%hu\n", status->tabs[i]);

  // items
  int current = 0;
  if (maxitems > 0 && status->selected > 0) {
	  // pagewise scrolling
	  current = status->selected - (status->selected % maxitems);
  }

  cSvdrpOsdItem *item = status->items.Get(current);
  while (ok && item && maxitems != 0) {
	  ok = Append("%c:%s\n", current == status->selected ? 'S' : 'I', item->Text());
	  current++;
	  maxitems--;
	  item = status->items.Next(item);
  }
  return ok;
}

const char* cPluginSvdrpOsd::Reply()
{
  char *tmp = buffer;
  buffer = NULL;
  return tmp;
}

void cPluginSvdrpOsd::Reset()
{
  free(buffer);
  buffer = NULL;
}

cString cPluginSvdrpOsd::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  // Process SVDRP commands this plugin implements
  if (!status)
	  return NULL;

  ReplyCode = 920;

  if (strcasecmp(Command, "LSTO") == 0) {
	  bool ok = true;

	  if (status->title)
		  ok = Append("T:%s\n", status->title);
	  if (ok && status->items.Count() > 0)
		  ok = AppendItems(Option);
	  if (ok && status->text)
		  ok = Append("X:%s\n", status->text);
	  if (ok && status->red)
		  ok = Append("R:%s\n", status->red);
	  if (ok && status->green)
		  ok = Append("G:%s\n", status->green);
	  if (ok && status->yellow)
		  ok = Append("Y:%s\n", status->yellow);
	  if (ok && status->blue)
		  ok = Append("B:%s\n", status->blue);
	  if (ok && status->message)
		  ok = Append("M:%s\n", status->message);
	  if (!ok) {
		  Reset();
		  ReplyCode = 451;
		  return cString::sprintf("Error while appending to buffer (size %d)", size);
	  }
	  if (buffer && length > 0)
		  return cString(Reply(), true);
  }
  else if (strcasecmp(Command, "OSDT") == 0) {
	  if (status->title)
		  return status->title;
  }
  else if (strcasecmp(Command, "OSDI") == 0) {
	  if (status->items.Count() > 0) {
		  if (AppendItems(Option)) {
			  return cString(Reply(), true);
		  }
		  else {
			  Reset();
			  ReplyCode = 451;
			  return cString::sprintf("Error while appending to buffer (size %d)", size);
		  }
	  }
  }
  else if (strcasecmp(Command, "OSDH") == 0) {
	  const char *red = status->red;
	  const char *green = status->green;
	  const char *yellow = status->yellow;
	  const char *blue = status->blue;
	  if (red || green || yellow || blue)
		  return cString::sprintf("R:%s\nG:%s\nY:%s\nB:%s",
				  red ? red : "", green ? green : "",
				  yellow ? yellow : "", blue ? blue : "");
  }
  else if (strcasecmp(Command, "OSDM") == 0) {
	  if (status->message)
		  return status->message;
  }
  else if (strcasecmp(Command, "OSDX") == 0) {
	  if (status->text)
		  return status->text;
  }
  else {
	  return NULL;
  }

  //Command recognized but nothing to reply
  ReplyCode = 930;
  return "Currently not on OSD";
}

VDRPLUGINCREATOR(cPluginSvdrpOsd); // Don't touch this!
