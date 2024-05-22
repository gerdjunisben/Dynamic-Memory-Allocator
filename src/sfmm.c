/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "debug.h"
#include "sfmm.h"
#define M 32


size_t maxPayloadSize = 0;
size_t currentPayLoadSize = 0;

void incrementPayload(size_t size)
{
    debug("The size %zu",size);
    currentPayLoadSize += size-8;
    if(currentPayLoadSize> maxPayloadSize)
    {
        maxPayloadSize = currentPayLoadSize;
    }
}

static inline void decreasePayload(size_t size)
{
    currentPayLoadSize -= size-8;
}

static inline size_t getVal(size_t val)
{
    return val^ MAGIC;
}

static void setVal(void* ptr,size_t val)
{
    *((size_t*)ptr) = val^ MAGIC;
}

static void incrementVal(void* ptr, size_t val)
{
    size_t ptrVal = getVal(*((size_t*)ptr));
    ptrVal += val;
    setVal(ptr, ptrVal);
}


static inline sf_header getSize(sf_block* block)
{
    return getVal(block->header)&0xFFFFFFF8;
}

static inline sf_footer* getFoot(sf_block* block)
{
    size_t size = getSize(block);
    void* ptr = block;
    return ptr + size;
}

static inline void setAlloc(sf_block* ourBlock)
{
    incrementVal(&ourBlock->header,2);
    incrementVal(getFoot(ourBlock),2);
}

static inline void insertInList(sf_block* sent,sf_block* newFree)
{
    newFree->body.links.next = sent->body.links.next;
    newFree->body.links.next->body.links.prev = newFree;
    sent->body.links.next = newFree;
    newFree->body.links.prev = sent;
}

static inline void removeFromList(sf_block* oldFree)
{
    sf_block* prev = oldFree->body.links.prev;
    sf_block* next = oldFree->body.links.next;
    next->body.links.prev =  prev;
    prev->body.links.next = next;
}

sf_block* getFromQuickList(int i)
{
    sf_block* current = sf_quick_lists[i].first;
    if(current == NULL)
    {
        return NULL;
    }
    else if(current->body.links.next ==NULL)
    {
        sf_quick_lists[i].length -=1;
        sf_quick_lists[i].first = NULL;
        //debug("%p",current);
        return current;
    }
    else
    {
        while(current->body.links.next->body.links.next != NULL)
        {
            current = current->body.links.next;
        }
        sf_block* temp = current->body.links.next;
        current->body.links.next = NULL;
        return temp;
    }
}


sf_block* findBlock(int size)
{
    debug("Lookin");
    if(size > sf_mem_end()-sf_mem_start())
    {
        return NULL;
    }
    //debug("size: %d",size);
    //quick list case
    if(size>=M && size<=M + ((M/2)*(NUM_QUICK_LISTS-1)))
    {
        //debug("attempting to find on quick list");
        sf_block* temp = NULL;
        for(int i = 0;i< NUM_QUICK_LISTS;i++)
        {
            if((M + i*(M>>1))>=size)
            {
                temp = getFromQuickList(i);
                if(temp !=NULL)
                {
                    incrementVal(&temp->header,-3);
                    setVal(getFoot(temp),getVal(temp->header));
                    return temp;
                }
            }
        }
    }
    debug("didn't find in quick");
    //normal list case
    int i = 1;
    unsigned int upper = M<<1;
    while(i<9 && upper<=size)
    {
        upper = upper<<1;
        i+=1;
    }
    while(i<10)
    {
        debug("%d",i);
        sf_block* sent = (sf_free_list_heads + i);
        sf_block* current = (sf_free_list_heads +i)->body.links.next;
        debug("sent %p, current %p",sent,current);
        while(sent != current)
        {
            debug("%zu",getSize(current));
            debug("%d",size);
            if(size <= getSize(current))
            {
                removeFromList(current);
                return current;
            }
            current = current->body.links.next;
        }
        i++;
    }
    return NULL;
}



void putInFreeList(sf_block* block)
{
    sf_header size = getSize(block);
    //debug("%zu",size);
    int i = 1;
    unsigned int upper = M <<1;
    unsigned int lower = M;
    if(size != M)
    {
        //debug("%zu %zu")
        while(i< 9 && !(lower<=size && upper>size))
        {
            lower = upper;
            upper = upper<<1;
            i+=1;
        }
    }
    else
    {
        i=0;
    }
    insertInList((sf_free_list_heads + i),block);

}

void coalesce(sf_block* blockA, sf_block* blockB)
{
    debug("blockA %p",blockA);
    debug("blockB %p",blockB);
    int sizeA = getSize(blockA);
    int sizeB = getSize(blockB);
    debug("A %d, B %d",sizeA,sizeB);

    removeFromList(blockA);
    removeFromList(blockB);
    //debug("break?");

    int newSize = sizeA + sizeB ;
    //int status = blockA->header&0x07;
    int status = getVal(blockA->header)&0x07;


    sf_block* newBlock = (void*)blockA;
    //newBlock->header = newSize + status;
    size_t n = newSize + status;
    setVal(&newBlock->header,n);
    //*getFoot(newBlock) = newSize + status;
    setVal(getFoot(newBlock),n);

    //sf_show_heap();
    putInFreeList(newBlock);
}

int coalesceSplit(sf_block* block)
{
    size_t size = getSize(block);
    void* ptr = block;
    sf_block* under = ptr + size;
    if(((getVal(under->header))&0x02) == 0)
    {
        coalesce(block, under);
        return 1;
    }
    return 0;
}

void coaAdjacent(void* ptr,sf_block* block, sf_header* nextHead,sf_footer* prvFoot)
{
    size_t prvFoorVal =getVal(*prvFoot);
    if(prvFoorVal> M && ((prvFoorVal)&0x02) == 0 && prvFoorVal<(sf_mem_end()-sf_mem_start()))
    {
        debug("attempting uppy");
        size_t upperSize = ((prvFoorVal)&0xFFFFFFF8);
        //debug("upper size %zu",upperSize);
        sf_block* upper = (ptr- upperSize);
        //debug("upper %p",upper);
        coalesce(upper,block);
        block = upper;
    }
    size_t nextHeadVal = getVal(*nextHead);
    //check if bottom is free if so coa
    if(nextHeadVal > M && ((nextHeadVal)&0x02) == 0 && nextHeadVal<(sf_mem_end()-sf_mem_start()))
    {
        debug("attempting down");
        sf_block* lower = (void*)nextHead - 8;
        //debug("lower %p",lower);
        coalesce(block,lower);
    }
}

void addToQuickList(sf_block* block,int i)
{
    sf_quick_lists[i].length +=1;
    if(sf_quick_lists[i].length == QUICK_LIST_MAX)
    {
        sf_block* current = sf_quick_lists[i].first;
        while(current->body.links.next != NULL)
        {
            current = current->body.links.next;
        }
        current->body.links.next = block;
        block->body.links.next = NULL;
        //sf_show_heap();
        current = sf_quick_lists[i].first;
        sf_block* next = current->body.links.next;
        while(current!= NULL)
        {
            debug("Loop");
            //sf_show_heap();
            debug("Current %p",current);
            debug("Next %p",next);
            void* ptr = current;
            size_t size = getSize(current);
            //ptr+=8;
            sf_header* head = ptr + 8;
            sf_footer* foot = ptr + size;
            //*head -=3;          //remove alloc and qckList bits
            incrementVal(head,-3);
            //*foot = *head;      //set the foot to the right thing
            setVal(foot,getVal(*head));
            sf_footer* prvFoot = ptr;
            sf_header* nextHead = ptr + size + 8;
            //sf_show_heap();
            putInFreeList(current);

            debug("Next head %zu, %p",*nextHead,nextHead);
            debug("Prv foot %zu, %p",*prvFoot,prvFoot);
            //check if top is free if so coa
            coaAdjacent(ptr,current,nextHead,prvFoot);


            sf_quick_lists[i].first = next;
            current = next;
            if(next!=NULL)
            {
                next = next->body.links.next;
            }
            //sf_show_heap();
        }
    }
    else
    {
        sf_block* current = sf_quick_lists[i].first;
        if(current == NULL)
        {
            //debug("set to first");
            sf_quick_lists[i].first = block;
            block->body.links.next = NULL;
            //debug("%p",sf_quick_lists[i].first);
        }
        else
        {
            while(current->body.links.next != NULL)
            {
                current = current->body.links.next;
            }
            current->body.links.next = block;
            block->body.links.next = NULL;
        }
    }
}

void split(sf_block* block,int size)
{
    void* ptr = (void*)block;
    sf_header blockSize = getSize(block);
    block = (sf_block*)ptr;
    //block->header = size + 2 + 4;
    setVal(&block->header,size + 6);
    //debug("%zu",getSize(block));
    //debug("%p",block);
    ptr += size;

    sf_block* free = (sf_block*)ptr;
    //debug("the size:%zu",(blockSize - size));
    sf_header offset = blockSize - size;
    //free->prev_footer = 0;
    setVal(&free->prev_footer,0);
    //free->header = offset+ 4;
    setVal(&free->header,offset+4);
    //debug("foot place %p",ptr);
    //*getFoot(free) = offset+ 4;
    setVal(getFoot(free), offset+4);
    debug("offset %zu",offset);
    putInFreeList(free);
    coalesceSplit(free);
    //debug("%zu",getSize(free));
    //debug("%p",free);

}




void setNewPageBlock(void* end,sf_footer prevFoot)
{
    end+=8;
    debug("%p",end);
    sf_header header = sf_mem_end() - end -12;
    sf_block* temp = (sf_block*)end;
    //temp->header = header;
    setVal(&temp->header,header);

    //temp->prev_footer = prevFoot;
    setVal(&temp->prev_footer,prevFoot);
    end = sf_mem_end()-16;
    sf_footer* foot = (sf_footer*)end;
    //*foot = header;
    setVal(foot,header);
    //debug("foot %zu, %p",*foot,foot);

    debug("head: %zu, %p",temp->header,&temp->header);
    debug("foot: %zu, %p", *foot,foot);
    putInFreeList(temp);
}

void setInitialBlock()
{
    void* start = sf_mem_start();
    start += 32;
    //debug("%p",start);
    sf_block* temp = (sf_block*)start;
    //temp->header = sf_mem_end() - sf_mem_start()-32-16 + 4;
    setVal(&temp->header,sf_mem_end() - sf_mem_start()-44);

    start = sf_mem_end() - 16;
    sf_footer* foot = (sf_footer*)start;
    //*foot = temp->header;
    setVal(foot,getVal(temp->header));
    //debug("%zu",temp->header);
    //debug("%zu", temp->prev_footer);
    putInFreeList(temp);
    //sf_show_block(temp);
}

void setEpilogue()
{
    void* end = sf_mem_end();
    //debug("%p",start);
    end -=8;
    //debug("%p",start);
    sf_header* temp = (sf_header*)end;
    //debug("%p",temp);
    setVal(temp,2);
    //*temp = 2;
    //debug("%zu",*temp);
    //debug("%zu",*((sf_header*)end));
}

void setPrologue()
{
    void* start = sf_mem_start();
    //debug("%p",start);
    start +=8;
    //debug("%p",start);
    sf_header* temp = (sf_header*)start;
    //debug("%p",temp);
    setVal(temp,38);
    //*temp = 38;
    //debug("%zu",*temp);
    //debug("%zu",*((sf_header*)start));
}

void initFree()
{
    for(int i = 0;i< NUM_QUICK_LISTS;i++)
    {
        sf_block* newDummy = (sf_free_list_heads + i);
        newDummy->body.links.next = newDummy;
        newDummy->body.links.prev = newDummy;
    }
}


void *sf_malloc(size_t size) {
    if (size == 0)
    {
        sf_errno = ENOMEM;
        return NULL;
    }

    if(sf_mem_start() == sf_mem_end())
    {
        //sf_set_magic(0x0);          ///////////////////////////////////////for testing make sure to remove

        //get a page
        sf_mem_grow();
        //init free list
        initFree();

        setPrologue();


        setEpilogue();

        setInitialBlock();
        //sf_show_heap();

    }
    //sf_show_heap();
    if(size < 16)
    {
        size = M;
        //debug("It got bumped up");
    }
    else if(size%16 != 0)
    {
        int c = 16;
        while(c< size +8)
        {
            c+= 16;
        }
        size = c;
    }
    else
    {
        size+=16;
    }
    sf_block* ourBlock = findBlock(size);
    //debug("block %p",ourBlock);
    //couldn't find a block implement properly later
    while(ourBlock == NULL)
    {
        void* end = sf_mem_end() - 24;
        if(!sf_mem_grow())
        {
            sf_errno = ENOMEM;
            return NULL;
        }
        setNewPageBlock(end, getVal(*(sf_footer*)(end+8)));
        setEpilogue();
        //sf_show_heap();
        sf_footer* foot = end+8;
        size_t footSize = getVal(*foot);
        if(((footSize)&0x02) == 0)
        {
            end-= (footSize)&0xFFFFFFF8;
            end+=8;
            sf_block* blockA = (sf_block*)end;
            end += (footSize)&0xFFFFFFF8;
            //end-=8;
            sf_block* blockB = (sf_block*)end;
            coalesce(blockA,blockB);
            //sf_show_heap();
        }
        //sf_show_heap();
        ourBlock = findBlock(size);
        //sf_show_heap();
    }
    sf_header blockSize =  getSize(ourBlock);
    //splitting is possible
    debug("Free block size %zu",blockSize);
    debug("Block size %zu",size);
    //sf_show_heap();
    if(blockSize - size >=M)
    {
        debug("split");
        //debug("block size %zu, size %zu",blockSize,size);
        split(ourBlock,size);
    }
    else
    {
        debug("set alloc");
        setAlloc(ourBlock);
    }
    //sf_show_heap();
    incrementPayload(getSize(ourBlock));
    //debug("%zu", sizeof(ourBlock));
    //debug("%p",ourBlock);
    return ((void*)ourBlock) + 16;
}

void sf_free(void *pp) {
    if(pp == NULL)
    {
        abort();
    }
    pp-=16;
    debug("%p",pp);
    sf_block* block = (sf_block*)pp;
    size_t size = getSize(block);
    sf_header* head = (pp +8);
    sf_footer* foot = (pp + size);      //where we want the foot to be
    sf_footer* prvFoot = (pp);
    sf_header* nextHead = (pp + size + 8);


    decreasePayload(size);

    //size_t nextSize = *(nextHead)&0xFFFFFFF8;
    size_t nextSize = getVal(*nextHead)&0xFFFFFFF8;
    sf_footer* nextFoot = (pp + size +nextSize);
    /*
    debug("size %zu",size);
    debug("%p",pp);
    debug("head %p",head);
    debug("%zu",*head);
    debug("feet %p",foot);
    debug("%zu",*foot);
    debug("prev feet %p",prvFoot);
    debug("%zu",*prvFoot);*/

    //check if it's aligned with modulo
    if(((size_t)(head))%16 ==0)
    {
        abort();
    }

    size_t headVal = getVal(*head);
    short prvAllo = (headVal)&0x04;
    short realPrvAllo = (getVal(*prvFoot))&0x02;
    short allo = (headVal)&0x02;
    short qklist = (headVal)&0x01;
    realPrvAllo = realPrvAllo<<1;
    debug("allo %d",allo);
    debug("prvAllo %d",prvAllo);
    debug("realPrvAllo %d",realPrvAllo);


    //compare prev alloc in header to the previous block alloc field to make sure they match
    if(!(getVal(*prvFoot)!=0 || *prvFoot!=0) && realPrvAllo != prvAllo )
    {
        abort();
    }

    //basic tests then
    //check if header is before the first block of the heap or the footer is before the last
    if(size < M || size%16 != 0 || allo == 0 || qklist == 1 || ((size_t)head < (size_t)(sf_mem_start()+24)) || ((size_t)foot > (size_t)(sf_mem_end()-8)))//test misc stuff
    {
        abort();
    }


    /////////actual freeing stuff//////////
    //invariant the block is a valid allocated block
    debug("pre inc");
    //sf_show_heap();
    //Set next prev alloc
    debug("nextHead %zu, nextFoot %zu",getVal(*nextHead),getVal(*nextFoot));
    if(((getVal(*nextHead))&0x04) == 4)
    {
        debug("nextHead %zu, nextFoot %zu",getVal(*nextHead),getVal(*nextFoot));
        //*nextHead -=4;
        incrementVal(nextHead,-4);
        debug("nextHead %zu, nextFoot %zu",getVal(*nextHead),getVal(*nextFoot));
        if(getVal(*nextFoot) != 0)
        {
            //*nextFoot -=4;
            incrementVal(nextFoot,-4);
        }
    }
    debug("nextHead %zu, nextFoot %zu",getVal(*nextHead),getVal(*nextFoot));
    //sf_show_heap();

    if(size>=M && size<=M + ((M/2)*(NUM_QUICK_LISTS-1)))
    {
        //*head = size + prvAllo + 2 + 1;
        debug("change");
        setVal(head,size + prvAllo + 3);
        debug("head %zu",getVal(*head));
        //sf_show_heap();

        for(int i = 0;i< NUM_QUICK_LISTS;i++)
        {
            if((M + i*(M/2))<=size && size<(M + (i+1) *(M/2)))
            {
                //debug("free list %d",i);
                addToQuickList(block,i);
                break;
            }
        }
    }
    else
    {
        //set block header and footer to free
        //*head = size + prvAllo;
        setVal(head,size + prvAllo);
        //*foot = size + prvAllo;
        setVal(foot,size + prvAllo);

        putInFreeList(block);
        coaAdjacent(pp,block,nextHead,prvFoot);


    }
    //sf_show_heap();

    //set pp to null
    pp = NULL;
}

void *sf_realloc(void *pp, size_t rsize) {
    if(rsize ==0)
    {
        pp-=16;
        debug("%p",pp);
        sf_block* block = (sf_block*)pp;
        size_t size = getSize(block);
        sf_header* head = (pp +8);
        sf_footer* foot = (pp + size);      //where we want the foot to be
        //sf_footer* prvFoot = (pp);
        sf_header* nextHead = (pp + size + 8);
        size_t nextSize = getVal(*nextHead)&0xFFFFFFF8;
        sf_footer* nextFoot = (pp + size +nextSize);
        size_t headVal = getVal(*head);
        short prvAllo = (headVal)&0x04;
        if(((getVal(*nextHead))&0x04) == 4)
        {
            debug("nextHead %zu, nextFoot %zu",getVal(*nextHead),getVal(*nextFoot));
            //*nextHead -=4;
            incrementVal(nextHead,-4);
            debug("nextHead %zu, nextFoot %zu",getVal(*nextHead),getVal(*nextFoot));
            if(getVal(*nextFoot) != 0)
            {
                //*nextFoot -=4;
                incrementVal(nextFoot,-4);
            }
        }
        setVal(head,size + prvAllo);
        //*foot = size + prvAllo;
        setVal(foot,size + prvAllo);

        putInFreeList(block);
        return NULL;
    }
    sf_block* ourBlock = pp -16;
    debug("pp %p, block %p",pp,ourBlock);
    size_t ourSize = getSize(ourBlock);
    size_t updatedRSize = 0;
    if(rsize < 16)
    {
        updatedRSize = M;
        //debug("It got bumped up");
    }
    else if(rsize%16 != 0)
    {
        int c = 16;
        while(c< rsize +8)
        {
            c+= 16;
        }
        updatedRSize = c;
    }
    else
    {
        updatedRSize= rsize + 16;
    }
    if(ourSize<updatedRSize)
    {
        void* newBlock = sf_malloc(rsize);
        if(newBlock == NULL)
        {
            return NULL;
        }
        memcpy(newBlock,pp,ourSize);
        sf_free(pp);
        return newBlock;
    }
    else if(ourSize>updatedRSize)
    {
        if(ourSize-updatedRSize <32)    ////causes splinter
        {
            return pp;
        }
        else    ////doesn't cause splinter
        {

            debug("ourSize %zu, rsize %zu",ourSize,updatedRSize);
            //ourBlock->header -= 2;
            incrementVal(&ourBlock->header,-2);
            sf_footer* foot = pp -16 + ourSize;
            debug("foot %p",foot);
            //*foot = ourBlock->header;
            setVal(foot,getVal(ourBlock->header));
            split(ourBlock,updatedRSize);
            //sf_show_heap();
            void* ptr = ourBlock;
            ptr+=16;
            debug("pp %p",ptr);
            return ptr;
        }
    }
    return pp;
}

///////////////////////leave unimplemented /////////////////////////////
double sf_fragmentation() {
    // To be implemented.
    abort();
}

double sf_utilization() {
    return ((1.0) * maxPayloadSize)/(sf_mem_end()-sf_mem_start());
}
