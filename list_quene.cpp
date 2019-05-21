
#include <iostream>
#include <string.h>

using namespace std;

class node_sjk
{
public:
    char * buffer;
    int bufferSize;
    node_sjk* next;

};

/*
typedef struct BufferDataNode
{
    uint8_t * buffer;
    int bufferSize;
    BufferDataNode * next;
} BufferDataNode;
*/
class Quene_sjk
{
public:

    pthread_mutex_t number_mutex;
    node_sjk *DataQueneHead;
    node_sjk *DataQueneTail;

    int node_size;

    Quene_sjk();

};

Quene_sjk::Quene_sjk()
{
    pthread_mutex_init(&number_mutex,NULL);

    DataQueneHead =NULL;
    DataQueneTail =NULL;

    node_size =0;
}

Quene_sjk Quene_one;

void DataQuene_Input(char * buffer,int size,Quene_sjk *quene)
{
    node_sjk * node = (node_sjk*)malloc(sizeof(node_sjk));
    node->buffer = (char *)malloc(size);
    node->bufferSize = size;
    node->next = NULL;

    memcpy(node->buffer,buffer,size);

    pthread_mutex_lock(&quene->number_mutex);

    if (quene->DataQueneHead == NULL)
    {
        quene->DataQueneHead = node;
    }
    else
    {
        quene->DataQueneTail->next = node;
    }

    quene->DataQueneTail = node;

    quene->node_size++;

    pthread_mutex_unlock(&quene->number_mutex);
}


static node_sjk *DataQuene_get(Quene_sjk *quene)
{
    node_sjk * node = NULL;

    pthread_mutex_lock(&quene->number_mutex);

    if (quene->DataQueneHead != NULL)
    {
        node = quene->DataQueneHead;

        if (quene->DataQueneTail == quene->DataQueneHead)
        {
            quene->DataQueneTail = NULL;
        }
        quene->node_size--;

        quene->DataQueneHead = quene->DataQueneHead->next;
    }

    pthread_mutex_unlock(&quene->number_mutex);

    return node;
}

int main()
{

    //pthread_mutex_init(&number_mutex,NULL);


    DataQuene_Input("FIRST",6,&Quene_one);
    DataQuene_Input("SECCOND",8,&Quene_one);

    node_sjk *BUFER;

    BUFER =DataQuene_get(&Quene_one);
    cout <<BUFER->buffer <<endl;

    BUFER =DataQuene_get(&Quene_one);
    cout <<BUFER->buffer <<endl;


}











