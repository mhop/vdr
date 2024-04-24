#include <string.h>
#include "status.h"

cSvdrpOsdStatus::cSvdrpOsdStatus() {
	title = message = text = NULL;
	red = green = yellow = blue = NULL;
	selected = -1;
	memset(&tabs, 0, sizeof(tabs));
}
cSvdrpOsdStatus::~cSvdrpOsdStatus() {
	OsdClear();
}

void cSvdrpOsdStatus::OsdClear() {
	free(title);
	free(message);
	free(red);
	free(green);
	free(yellow);
	free(blue);
	free(text);
	title = message = text = NULL;
	red = green = yellow = blue = NULL;
	items.Clear();
	selected = -1;
	memset(&tabs, 0, sizeof(tabs));
}

void cSvdrpOsdStatus::OsdTitle(const char *Title) {
	free(title);
	title = Title ? strdup(Title) : NULL;
}

void cSvdrpOsdStatus::OsdStatusMessage(const char *Message) {
	free(message);
	message = Message ? strdup(Message) : NULL;
}

void cSvdrpOsdStatus::OsdHelpKeys(const char *Red, const char *Green, const char *Yellow, const char *Blue) {
	free(red);
	red = Red ? strdup(Red) : NULL;
	free(green);
	green = Green ? strdup(Green) : NULL;
	free(yellow);
	yellow = Yellow ? strdup(Yellow) : NULL;
	free(blue);
	blue = Blue ? strdup(Blue) : NULL;
}

void cSvdrpOsdStatus::OsdItem(const char *Text, int Index) {
	const char* tab;
	const char* colStart = Text;
	for (int col = 0; col < MaxTabs &&
			(tab = strchr(colStart, '\t')); col++) {
		int width = tab - colStart + 1;
		if (width > tabs[col])
			tabs[col] = width;
		colStart = colStart + width;
	}
	items.Add(new cSvdrpOsdItem(Text));
}

void cSvdrpOsdStatus::OsdCurrentItem(const char *Text) {
	int i = -1;
	int best = -1;
	int dist = items.Count();
	cSvdrpOsdItem * currentItem = NULL;
	for (cSvdrpOsdItem *item = items.First(); item; item = items.Next(item)) {
		if (++i == selected)
			currentItem = item;
		if (strcmp(item->Text(), Text) == 0) {
			if (abs(i - selected) < dist) {
				// best match is the one closest to previous position
				best = i;
				dist = abs(i - selected);
			}
			else if (selected < 0) {
				// previous position unknown - take first match
				best = i;
				break;
			}
			else {
				// we already have a better match, so we're done
				break;
			}
		}
	}
	if (best >= 0) {
		// found matching item
		selected = best;
	}
	else if (currentItem) {
		// no match: the same item is still selected but its text changed
		currentItem->Update(Text);
	}
}

void cSvdrpOsdStatus::OsdTextItem(const char *Text, bool Scroll) {
	free(text);
	if (Text) {
		text = strdup(Text);
		strreplace(text, '\n', '|');
	}
	else {
		text = NULL;
	}
}
