#ifndef __LIME_LIST_H__
#define __LIME_LIST_H__

typedef struct lime_list_entry
{
	void *data;
	struct lime_list_entry *next;

} LimeListEntry;

typedef struct lime_list
{
	LimeListEntry *root;
} LimeList;

LimeList *lime_list_create();

int lime_list_add(LimeList *list, void *data);

void lime_list_del(LimeList *list, void *data);

void lime_list_destory(LimeList *list);

void lime_list_entry_destory(LimeListEntry *entry);

#endif