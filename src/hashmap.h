/*
CSTHASH - template for hash class

Copyright (C) 2012 Center for Sprogteknologi, University of Copenhagen

This file is part of CSTHASH.

CSTHASH is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

CSTHASH is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with CSTHASH; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <assert.h>
#include <stddef.h>
#include <string.h>

extern size_t nextprime(size_t g);
extern size_t casesensitivehash(const char * cp);


template <class record> class chainLink;

template <class record> class hash
    {
    private:
        typedef const char * (record::*keyFnc)() const;
        typedef void (record::*forallfunc)();
        typedef bool (record::*diffFnc)(void * p) const;
        keyFnc keyf;
        size_t hash_size;
        size_t record_count;
        chainLink<record> ** hash_table;

        size_t key(const char * ckey) const
            {
            return ((/*unsigned*/ size_t)casesensitivehash(ckey)) % hash_size;
            }
        void inc(){++record_count;}
        void dec(){--record_count;}
        void rehash(size_t loadFactor/*1-100*/)
            {
//            checkIntegrity();
            size_t oldsize = hash_size;
            hash_size = nextprime((100 * record_count)/loadFactor);
            chainLink<record> ** new_hash_table = new chainLink<record> * [hash_size];
            size_t i;
            for(i = 0;i < hash_size;++i)
                new_hash_table[i] = 0;
            if(hash_table)
                {
                for(i = oldsize;i > 0;)
                    {
                    chainLink<record> * r = hash_table[--i];
                    while(r)
                        {
                        size_t Key = key((r->thing->*keyf)());
                        chainLink<record> ** bucket = new_hash_table + Key;
                        chainLink<record> * Next = r->Next;
                        r->Next = *bucket;
                        *bucket = r;
                        r = Next;
                        }
                    }
                }
            delete [] hash_table;
            hash_table = new_hash_table;
            }
        size_t loadfactor() const
            {
            return (record_count < 10000000L) 
                ? (100 * record_count) / hash_size 
                : record_count / (hash_size/100);
            }
    public:
        hash(keyFnc keyf,size_t size):keyf(keyf),record_count(0)
            {
            hash_size = nextprime(size);
            hash_table = new chainLink<record> * [hash_size];
            size_t i;
            for(i = 0;i < hash_size;++i)
                hash_table[i] = 0;
            }
        ~hash()
            {
            size_t i;
            for(i = 0;i < hash_size;++i)
                {
                chainLink<record> * rec = hash_table[i];
                while(rec)
                    {
                    chainLink<record> * victim = rec;
                    rec = rec->Next;
                    delete victim;
                    }
                }
            delete [] hash_table;
            }
        record * find(const char * ckey,void *& bucket) const
            {
//            checkIntegrity();
            size_t Key = key(ckey);
            (chainLink<record> **&)bucket = hash_table + Key;
            chainLink<record> * p;
            if(!strcmp(ckey,"[...]"))
                p = *(chainLink<record> **&)bucket;
//            checkIntegrity();
            for(p = *(chainLink<record> **&)bucket;p && strcmp((p->thing->*keyf)(),ckey);p = p->Next)
                ;
            if(p)
                {
                (chainLink<record> **&)bucket = &p->Next;
                return p->thing;
                }
            else
                return 0;
            }
/*        void checkIntegrity() const
            {
            size_t i;
            for(i = 0;i < hash_size;++i)
                {
                chainLink<record> * p;
                for(p = *(chainLink<record> **)(hash_table + i);p;p = p->Next)
                    assert(p->thing);
                }
            }*/
        record * findNext(const char * ckey,void *& bucket) const
            {
//            checkIntegrity();
            chainLink<record> * p;
            for(p = *(chainLink<record> **&)bucket;p && strcmp((p->thing->*keyf)(),ckey);p = p->Next)
                ;
            if(p)
                {
                (chainLink<record> **&)bucket = &p->Next;
                return p->thing;
                }
            else
                return 0;
            }
        void insert(record * rec,void *& buck)
            {
//            checkIntegrity();
            assert(rec != 0);
            size_t lf = loadfactor();
            if(lf > 100)
                {
                rehash(60);
                buck = 0;
                }

            chainLink<record> ** bucket = (chainLink<record> **)buck;
            if(bucket == 0)
                {
                size_t Key = key((rec->*keyf)());
                buck = bucket = hash_table + Key;
                }
            chainLink<record> * link = new chainLink<record>(rec);
            link->Next = *bucket;
            *bucket = link;
            inc();
            }
        void remove(record * rec,void *& buck)
            {
//            checkIntegrity();
            size_t lf = loadfactor();
            if(lf < 20)
                {
                rehash(60);
                buck = 0;
                }

            chainLink<record> ** bucket = (chainLink<record> **)buck;
            if(bucket == 0)
                {
                size_t Key = key((rec->*keyf)());
                buck = bucket = hash_table + Key;
                }
            chainLink<record> ** p = bucket;
            while(*p)
                {
                if((*p)->thing == rec)
                    {
                    chainLink<record> * victim = *p;
                    *p = (*p)->Next;
                    delete victim;
                    dec();
                    return;
                    }
                p = &(*p)->Next;
                }
            }
        void deleteThings()
            {
//            checkIntegrity();
            size_t i;
            for(i = 0;i < hash_size;++i)
                {
                chainLink<record> * rec = hash_table[i];
                while(rec)
                    {
                    chainLink<record> * victim = rec;
                    rec = rec->Next;
                    delete victim->thing;
                    delete victim;
                    dec();
                    }
                hash_table[i] = 0;
                }
            }
        size_t forall(forallfunc fnc) const
            {
//            checkIntegrity();
            size_t i;
            size_t n = 0;
            for(i = 0;i < hash_size;++i)
                {
                chainLink<record> * rec = hash_table[i];
                while(rec)
                    {
                    ++n;
                    (rec->thing->*fnc)();
                    rec = rec->Next;
                    }
                }
            return n;
            }
        record ** convertToList(size_t & N)
            {
//            checkIntegrity();
            N = record_count;
            record ** ret = new record * [record_count];
            size_t i;
            size_t n = 0;
            for(i = 0;i < hash_size;++i)
                {
                chainLink<record> * rec = hash_table[i];
                while(rec)
                    {
                    ret[n++] = rec->thing;
                    rec = rec->Next;
                    }
                }
            return ret;
            }
    };

template <class record> class chainLink
    {
    private:
        friend class hash<record>;
        chainLink<record> * Next; // records with the same hash.
        record * thing;
    public:
    public:
        chainLink(record * thing):Next(0),thing(thing)
            {
            assert(thing);
            }
        ~chainLink(){}
    };




