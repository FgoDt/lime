#include "list.h"
#include "mem.h"

LimeList *lime_list_create()
{
	LimeList *l = lime_mallocz(sizeof(*l));
	return l;
}

int lime_list_add(LimeList *list, void *data)
{
	LimeListEntry *entry = lime_mallocz(sizeof(*entry));
	entry->data = data;
	entry->next = list->root;
	list->root = entry;
}

void lime_list_del(LimeList *list, void *data)
{
	LimeListEntry *entry = NULL;
	LimeListEntry *pre = NULL;
	for (entry = list->root; entry != NULL; entry = entry->next)
	{
		if (data == entry->data)
		{
			break;
		}
		pre = entry;
	}
	if (entry == NULL)
	{
		return;
	}
	if (entry == list->root)
	{
		list->root = entry->next;
		return;
	}
	pre->next = entry->next;
	lime_list_entry_destory(entry);
	return;
}

void lime_list_entry_destory(LimeListEntry *entry)
{
	if (entry == NULL)
	{
		return;
	}
	lime_free(entry);
}

void lime_list_destory(LimeList *list)
{
	if (!list)
	{
		return;
	}
	for (LimeListEntry *entry = list->root; entry != NULL; entry = entry->next)
	{
		lime_list_entry_destory(entry);
	}
	lime_free(list);
}