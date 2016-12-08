/*
 * The MIT License (MIT)
 * Copyright (C) 2016 Marco Guerri
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of 
 * this software and associated documentation files (the "Software"), to deal in 
 * the Software without restriction, including without limitation the rights to use, 
 * copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the 
 * Software, and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all 
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION 
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <assert.h>
#include "list.h"

#define LIST_PRINT_BUFF_SIZE        16
#define REALLOC_THRESHOLD           8


/**
 * @brief Frees a node and associated dynamically allocated memory
 */
void 
free_node(list_node_t* ptr_node)
{
    free(ptr_node->data->payload);
    free(ptr_node->data);
    free(ptr_node);
}


/* Initializes a new root node */
list_node_t*
list_init(void *payload, size_t size)
{
    if(payload == NULL || size <= 0)
        return NULL;

    list_data_t *ptr_data = (list_data_t*)malloc(sizeof(list_data_t));
    if(ptr_data == NULL)
    {
        perror("malloc");
        goto err_data;
    }

    ptr_data->size = size;
    ptr_data->payload = (void*)malloc(size);
    if(ptr_data->payload == NULL)
    {
        perror("malloc");
        goto err_payload;
    }

    memcpy(ptr_data->payload, payload, size);

    list_node_t *ptr_node = (list_node_t*)malloc(sizeof(list_node_t));
    if(ptr_node == NULL) 
    {
        perror("malloc");
        goto err_node;
    
    }
    ptr_node->data = ptr_data;
    ptr_node->next = NULL;
    ptr_node->prev = NULL;

    assert(memcmp(payload, ptr_node->data->payload, size) == 0);
    assert(ptr_node->next == NULL);
    assert(ptr_node->prev == NULL);

    return ptr_node;

err_node:
    free(ptr_node);
err_payload:
    free(ptr_data->payload); 
err_data:
    free(ptr_data);
    return NULL;
}


char*
list_print(list_node_t *root, int (*print_payload)(void*, char*))
{
    size_t curr_buff_size = LIST_PRINT_BUFF_SIZE;
    char *buff = (char*)malloc(sizeof(char)*LIST_PRINT_BUFF_SIZE);
    memset(buff, 0, LIST_PRINT_BUFF_SIZE);

    if(print_payload == NULL)
        return NULL;

    int written = 0;
    size_t buff_ptr = 0;
    while(root != NULL)
    {
        written =  (*print_payload)(root->data->payload, buff + buff_ptr);
        if(written == -1)
        {
            free(buff);
            return NULL;
        }

        buff_ptr += written;
        if(buff_ptr  > curr_buff_size - REALLOC_THRESHOLD)
        {
            char *new_buff = (char*)realloc(buff, sizeof(char)*(curr_buff_size*2));
            if(new_buff == NULL) 
            {
                free(buff); /* Old buffer was still allocated */
                perror("realloc");
                return NULL;
            }
            buff = new_buff;
            memset(buff + buff_ptr, 0, curr_buff_size);
            curr_buff_size = curr_buff_size*2;
        }
        root = root->next;
    }
    return buff;
}


/**
 * @brief free root node and all those following
 * @param root Root node. This might not be necessarily the root of the list.
 * All the nodes following root, including root, will be freed
 */
void
list_destroy(list_node_t *root)
{
    /* root might not be the root of the list. In this case, set the next pointer
     * of the previous node to NULL as root will be freed */
    if(root->prev != NULL)
        root->prev->next = NULL;

    while(root != NULL)
    {
        list_node_t* next = root->next;
        free(root->data->payload);
        free(root->data);
        free(root);
        root = next;
    }
}

/**
 * Returns the size of the list starting by the node passed as argument,
 * which might not be the root. TODO: Consider improving by keeping temporary
 * counters.
 */
size_t 
list_len(list_node_t* root)
{
    size_t len = 0;
    while(root != NULL)
    {
        ++len;
        root = root->next;
    }
    return len;
}

/*
 * @brief Inserts a new node in position pos starting from ptr_root. 
 * @param pos Position (0-indexed) where to add the new node.
 * @return Pointer to the new list root or NULL upon failure. When returning
 * NULL the old list is NOT destroyed.
 */
list_node_t*
list_insert(list_node_t* ptr_root, void *payload, size_t size, size_t pos)
{
    if(ptr_root == NULL || payload == NULL || pos > list_len(ptr_root))
        return NULL;
    
    list_node_t *ptr_prev = NULL, *ptr_pos = ptr_root;
    while(pos > 0)
    {
        ptr_prev = ptr_pos;
        /* Checked pos against the length of the list, we can't go beyond the 
         * last element */
        assert(ptr_pos != NULL);
        ptr_pos = ptr_pos-> next;
        --pos;
    }

    list_node_t *ptr_node = (list_node_t*)malloc(sizeof(list_node_t));
    list_data_t* ptr_data = (list_data_t*)malloc(sizeof(list_data_t));
    ptr_data->payload = (void*)malloc(size);

    
    if(ptr_node == NULL || ptr_data == NULL || ptr_data->payload == NULL)
    {
        perror("malloc");
        goto err_node_data;
    }

    if(ptr_pos == NULL) 
    {
        /* Appending at the end */
        assert(ptr_node != NULL && ptr_prev != NULL);
        ptr_node->next = NULL;
        ptr_node->prev = ptr_prev;
        ptr_prev->next = ptr_node;
    }
    else if(ptr_pos->prev == NULL)
    {
        /* Beginning of the list */
        assert(ptr_node != NULL && ptr_pos != NULL);
        ptr_node->next = ptr_root;
        ptr_node->prev = NULL;
        ptr_pos->prev = ptr_node;
        /* The node just allocated is the new root which will be returned */
        ptr_root = ptr_node;
    }
    else
    {  
        /* Adding in the middle */ 
        assert(ptr_node != NULL && ptr_pos != NULL && ptr_pos->prev != NULL);
        ptr_node->next = ptr_pos;
        ptr_node->prev = ptr_pos->prev;
        ptr_pos->prev = ptr_node;
        ptr_pos->prev->next = ptr_node;
    }

    memcpy(ptr_data->payload, payload, size);
    ptr_data->size = size;
    ptr_node->data = ptr_data;
    assert(memcmp(payload, ptr_node->data->payload, size) == 0);

    return ptr_root;

err_node_data:
    free_node(ptr_node);
    return NULL;
}

/**
 * @brief Deletes the first node which matches the payload passed as argument
 * @param payload Paylod to delete
 * @param size Size of the payload
 * @return The pointer to the new list
 */
list_node_t*
list_del(list_node_t* ptr_root, void* payload, size_t size)
{
    list_node_t *ptr_node = ptr_root;
    while(ptr_node != NULL) 
    {
        if(memcmp(ptr_node->data->payload, payload, size) == 0)
        {
            if(ptr_node->prev == NULL)
            {
                /* First node of the list */
                if(ptr_node->next != NULL)
                {
                   /* First node is followed by at least one more node */
                   ptr_node->next->prev = NULL;
                   list_node_t* temp = ptr_node->next;
                   free_node(ptr_node);
                   return temp;
                }
                else
                {
                    /* First node is the only one in the list */
                    free_node(ptr_node);
                    return NULL;
                }
            }
            else
            {
                /* Node is not the first in the list */
                if(ptr_node->next != NULL)
                {
                    /* Node is followed by at least one more node */
                    ptr_node->next->prev = ptr_node->prev;
                    ptr_node->prev->next = ptr_node->next;
                    free_node(ptr_node);

                }
                else
                { 
                    /* Node is the last in the list */
                    ptr_node->prev->next = NULL;
                    free_node(ptr_node);
                }
                return ptr_root;
            }

        }
        ptr_node = ptr_node->next;
    }
    /* Should never reach here */
    assert(0);
}


/**
 * @brief Returns a pointer to the first occurrence of payload in the list
 * @param ptr_root Pointer to root of the list
 * @param payload Payload to be searched in the list
 * @return Pointer to the first node matching the payload or NULL if payload
 * is not found
 */
list_node_t*
list_search(list_node_t* ptr_root, void* payload, size_t size)
{

    while(ptr_root != NULL)
    {
        if(memcmp(ptr_root->data->payload, payload, size) == 0)
            return ptr_root;
        else
            ptr_root = ptr_root->next;
    }
    return NULL;   
}


/**
 * @brief Returns a pointer to the payload of the n-th element of the list
 * @param ptr_root Pointer to root of the list
 * @param pos 0-indexed position of the element to return
 * @return Pointer to the payload of the n-th element of the list or NULL upon
 * failure
 */
void*
list_get(list_node_t *ptr_root, size_t pos)
{

    if(ptr_root == NULL || pos >= list_len(ptr_root))
        return NULL;
    while(pos > 0)
    {
        /* We checked pos against the length of the list, can't be null at this
         * point */
        assert(ptr_root != NULL);
        ptr_root = ptr_root->next;
        --pos;
    }
    return ptr_root->data->payload;

}

