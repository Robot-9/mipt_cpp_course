#include "event_list.h"

namespace nano_edr {

void ListPopFront(EventList* list) {
    if (list == nullptr || list->head == nullptr) {
        return;
    }

    EventNode* node = list->head;
    list->head = node->next;
    delete node;
    --list->size;

    if (list->head == nullptr) {
        list->tail = nullptr;
    }
}

void ListPushBack(EventList* list, const Event* event) {
    if (list == nullptr || event == nullptr) {
        return;
    }

    while (list->capacity != 0 && list->size >= list->capacity) {
        ListPopFront(list);
    }

    EventNode* node = new EventNode{*event, nullptr};

    if (list->tail == nullptr) {
        list->head = node;
        list->tail = node;
    } else {
        list->tail->next = node;
        list->tail = node;
    }

    ++list->size;
}

void ListClear(EventList* list) {
    if (list == nullptr) {
        return;
    }

    while (list->head != nullptr) {
        ListPopFront(list);
    }
}

EventList::~EventList() {
    ListClear(this);
}

}  // namespace nano_edr