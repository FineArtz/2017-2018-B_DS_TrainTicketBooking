//
// Created by FineArtz on 2018/05/08
//

#ifndef INC_2018DATASTRUCTUREBIGWORK_BPLUSTREE_H
#define INC_2018DATASTRUCTUREBIGWORK_BPLUSTREE_H

#include <cstdio>
#include <fstream>
#include <functional>
#include <cstring>

namespace myAlgorithm{

static const char WRITE_PATH[256] = "../../tmp/records.txt";
static const int UNIT = 4096;

template <  
    class Key, 
    class Val,
    class Compare = std::less<Key> 
>
class BPlusTree{
public:
    using key_type          = Key;
    using value_type        = Val;
	using key_compare       = Compare;
	using size_type         = size_t;
    using difference_type   = std::ptrdiff_t;

private:
    struct Index{
        Key key;
        off_t child;
    };

    struct Record{
        Key key;
        Val value;
    };

    static const size_t M = (4096 - 3 * sizeof(off_t) - sizeof(size_t)) / (sizeof(Index));
    static const size_t L = (4096 - 3 * sizeof(off_t) - sizeof(size_t)) / (sizeof(Record));

    struct LeafNode{
        off_t parent = 0;
        off_t prev = 0, succ = 0;
        size_t size = 0;
        Record record[L];
    };

    struct TreeNode{
        off_t parent = 0;
        off_t prev = 0, succ = 0;
        size_t size = 0;
        Index index[M];
    };

private:
    char path[1024];
    mutable FILE *fp = nullptr;
    mutable int fileLevel = 0;

    off_t root = 0, slot = 0;
    size_t _size = 0;
    int height = 0;

    Compare comp;

#ifndef _NO_DEBUG
private:
#else
public:
#endif
    void openFile(const char *mode = "rb+") const{
        if (fileLevel == 0)
            fp = fopen(path, mode);
        ++fileLevel;
    }
    
    void closeFile() const{
        if (fileLevel == 1)
            fclose(fp);
        --fileLevel;
    }

private:
    off_t alloc(){
        off_t s = slot;
        slot += UNIT;
        return s;
    }

    template <class T>
    size_t read(T *val, off_t offset){
#ifndef _NO_DEBUG
        openFile();
#endif
        fseek(fp, offset, SEEK_SET);
        char ch[UNIT];
        size_t s = fread(ch, UNIT, 1, fp);
        *val = *(reinterpret_cast<T *>(ch));
#ifndef _NO_DEBUG
        closeFile();
#endif
        return s;
    }

    template <class T>
    size_t write(T *val, off_t offset, char *mode = "rb+"){
#ifndef _NO_DEBUG
        openFile(mode);
#endif
        fseek(fp, offset, SEEK_SET);
        char *ch;
        ch = reinterpret_cast<char *>(val);
        size_t s = fwrite(ch, UNIT, 1, fp);
#ifndef _NO_DEBUG
        closeFile();
#endif
        return s;
    }

private:
    Index *begin(TreeNode &tn){
        return tn.index;
    }
    Record *begin(LeafNode &ln){
        return ln.record;
    }
    Index *end(TreeNode &tn){
        return tn.index + tn.size;
    }
    Record *end(LeafNode &ln){
        return ln.record + ln.size;
    }

    void copyIndex(Index *first, Index *last, Index *des){
        while (first != last){
            *des = *first;
            ++first;
            ++des;
        }
    }
    void copyRecord(Record *first, Record *last, Record *des){
        while (first != last){
            *des = *first;
            ++first;
            ++des;
        }
    }
    void copyBackIndex(Index *first, Index *last, Index *des){
        while (last != first){
            *(des - 1) = *(last - 1);
            --last;
            --des;
        }
    }
    void copyBackRecord(Record *first, Record *last, Record *des){
        while (last != first){
            *(des - 1) = *(last - 1);
            --last;
            --des;
        }
    }

    Index *binarySearchKey(TreeNode &tn, const Key &key){
        if (tn.size == 0)
            return end(tn);
        size_t l = 0, r = tn.size - 1, mid;
        while (l < r){
            mid = (l + r) / 2;
            if (comp(tn.index[mid].key, key))
                l = mid + 1;
            else
                r = mid;
        }
        return tn.index + l;
    }

    Record *binarySearchRecord(LeafNode &ln, const Key &key){
        if (ln.size == 0)
            return end(ln);
        size_t l = 0, r = ln.size - 1, mid;
        while (l < r){
            mid = (l + r) / 2;
            if (comp(ln.record[mid].key, key))
                l = mid + 1;
            else
                r = mid;
        }
        return ln.record + l;
    }

    void updateParentIndex(Index *first, Index *last, off_t newParent){
        TreeNode tn;
        while (first != last){
            read(tn, first->child);
            tn.parent = newParent;
            write(tn, first->child);
            ++first;
        }
    }

    void updateChildIndex(off_t parent, const Key &oldKey, const Key &newKey){
        if (parent == 0)
            return;
        TreeNode tn;
        read(&tn, parent);
        if (tn.size == 0) {
            tn.index[0].key = newKey;
            ++tn.size;
            write(&tn, parent);
            updateChildIndex(tn.parent, oldKey, newKey);
        }
        else {
            Index *id = binarySearchKey(tn, oldKey);
            if (id == end(tn))
                return;
            id->key = newKey;
            write(&tn, parent);
            if (id == end(tn) - 1)
                updateChildIndex(tn.parent, oldKey, newKey);
        }
    }

    bool getKeyFromRight(TreeNode &t, off_t offset){
        off_t pos = t.succ;
        TreeNode rn;
        read(&rn, pos);
        Index *readPos = begin(rn), *writePos = end(t);
        if (rn.size <= M * 3 / 2)
            return false;

        TreeNode parent;
        read(&parent, t.parent);
        Index *keyInParent = binarySearchKey(parent, t.index[t.size - 1].key);
        keyInParent->key = readPos->key;
        write(&parent, t.parent);

        *writePos = *readPos;
        ++t.size;

        updateParentIndex(readPos, readPos + 1, offset);
        copyIndex(readPos + 1, end(rn), readPos);
        --rn.size;
        write(rn, pos);
        return true;
    }

    bool getKeyFromLeft(TreeNode &t, off_t offset){
        off_t pos = t.prev;
        TreeNode rn;
        read(&rn, pos);
        Index *readPos = end(rn) - 1, *writePos = begin(t);
        if (rn.size <= M * 3 / 2)
            return false;

        TreeNode parent;
        read(&parent, t.parent);
        Index *keyInParent = binarySearchKey(parent, rn.index[0].key);
        writePos->key = keyInParent->key;
        keyInParent->key = (readPos - 1)->key;
        write(&parent, t.parent);

        copyBackIndex(writePos, end(t), end(t) + 1);
        *writePos = *readPos;
        ++t.size;

        updateParentIndex(readPos, readPos + 1, offset);
        copyIndex(readPos + 1, end(rn), readPos);
        --rn.size;
        write(rn, pos);
        return true;
    }

    bool getRecordFromRight(LeafNode &t){
        off_t pos = t.succ;
        LeafNode rn;
        read(&rn, pos);
        Record *readPos = begin(rn), *writePos = end(t);
        if (rn.size <= L * 2 / 3)
            return false;

        updateChildIndex(t.parent, t.record[0].key, rn.record[1].key);
        *writePos = *readPos;
        ++t.size;

        copyRecord(readPos + 1, end(rn), readPos);
        --rn.size;
        write(&rn, pos);
        return true;
    }

    bool getRecordFromLeft(LeafNode &t){
        off_t pos = t.prev;
        LeafNode rn;
        read(&rn, pos);
        Record *readPos = end(rn) - 1, *writePos = begin(t);
        if (rn.size <= L * 2 / 3)
            return false;

        updateChildIndex(rn.parent, rn.record[0].key, readPos->key);
        copyBackRecord(writePos, end(t), end(t + 1));
        *writePos = *readPos;
        ++t.size;

        copyRecord(readPos + 1, end(rn), readPos);
        --rn.size;
        write(&rn, pos);
        return true;
    }

    void removeIndex(off_t offset, const Key &key){
        TreeNode tn;
        read(&tn, offset);
        if (tn.size == 0)
            return;
        int l = 0, r = tn.size - 1, mid;
        while (l < r){
            mid = (l + r) >> 1;
            if (comp(tn.index[mid].key, key))
                l = mid + 1;
            else
                r = mid;
        }
        copyIndex(begin(tn) + l + 1, end(tn), begin(tn) + l);
        --tn.size;
        if (tn.succ != 0){
            TreeNode sib;
            read(&sib, tn.succ);
            if (tn.size + sib.size <= M){
                mergeKey(tn, sib);
                if (tn.parent == root && tn.prev == 0 && tn.succ == 0){
                    root = offset;
                    tn.parent = 0;
                    --height;
                }
            }
        }
        write(&tn, offset);
    }

    void createTreeNode(off_t offset, TreeNode *tn, TreeNode *nx){
        nx->parent = tn->parent;
        nx->succ = tn->succ;
        nx->prev = offset;
        tn->succ = alloc();
        if (nx->succ != 0){
            TreeNode on;
            read(&on, nx->succ);
            on.prev = tn->succ;
            write(&on, nx->succ);
        }
    }
    void createLeafNode(off_t offset, LeafNode *ln, LeafNode *nx){
        nx->parent = ln->parent;
        nx->succ = ln->succ;
        nx->prev = offset;
        ln->succ = alloc();
        if (nx->succ != 0){
            LeafNode on;
            read(&on, nx->succ);
            on.prev = ln->succ;
            write(&on, nx->succ);
        }
    }
    void removeTreeNode(TreeNode *pr, TreeNode *tn){
        pr->succ = tn->succ;
        if (pr->succ != 0){
            TreeNode on;
            read(&on, tn->succ);
            on.prev = tn->prev;
            write(&on, tn->succ);
        }
        removeIndex(tn->parent, tn->index[tn->size - 1].key);
    }
    void removeLeafNode(LeafNode *pr, LeafNode *ln){
        pr->succ = ln->succ;
        if (pr->succ != 0){
            LeafNode on;
            read(&on, ln->succ);
            on.prev = ln->prev;
            write(&on, ln->succ);
        }
        removeIndex(ln->parent, ln->record[ln->size - 1].key);
    }

    void mergeLeaf(LeafNode *des, LeafNode *from){
        copyRecord(begin(*from), end(*from), end(*des));
        des->size += from->size;
        removeLeafNode(des, from);
    }
    void mergeKey(TreeNode &des, TreeNode &from){
        copyIndex(begin(from), end(from), end(des));
        des.size += from.size;
        removeTreeNode(&des, &from);
    }

    off_t findKey(const Key &key, bool mode = false;) {
        TreeNode tn;
        read(&tn, root);
        if (tn.size == 0)
            return tn.index[0].child;
        Index *id;
        for (int i = 1; i <= height - 2; ++i){
            id = binarySearchKey(tn, key);
            read(&tn, id->child);
        }
        id = binarySearchKey(tn, key);
        if (mode)
            return id->child;
        if (id->key != key)
            return 0;
        else
            return id->child;
    }

    void insertNewIndex(off_t offset, const Key &key, off_t child){
        TreeNode tn;
        read(&tn, offset);
        if (tn.size != M){
            int i;
            for (i = tn.size; i > 0; --i){
                if (!comp(key, tn.index[i - 1].key))
                    break;
                tn.index[i] = tn.index[i - 1];
            }
            tn.index[i].key = key;
            tn.index[i].child = child;
            if (i == tn.size){
                updateChildIndex(tn.parent, tn.index[i - 1].key, key);
            }
            ++tn.size;
            write(&tn, offset);
        }
        else if (tn.succ != 0){
            TreeNode sib;
            read(&sib, tn.succ);
            if (sib.size != M){
                copyBackIndex(begin(sib), end(sib), end(sib) + 1);
                copyIndex(end(tn) - 1, end(tn), begin(sib));
                ++sib.size;
                write(&sib, tn.succ);
                int i = tn.size;
                tn.index[i - 1].key = key;
                tn.index[i - 1].child = child;
                write(&tn, offset);
            }
            else{
                TreeNode newNode;
                int newPos = alloc();
                createTreeNode(newPos, &newNode, &sib);
                Key old = tn.index[tn.size - 1].key;
                copyIndex(end(tn) - M / 3, end(tn), begin(newNode));
                copyIndex(begin(sib), begin(sib) + M / 3, end(newNode));
                tn.size -= M / 3;
                copyIndex(begin(sib) + M / 3 + 1, end(sib), begin(sib));
                sib.size -= M / 3;
                newNode.size = M / 3 * 2;
                updateChildIndex(tn.parent, old, tn.index[tn.size - 1].key);
                insertNewIndex(tn.parent, newNode.index[newNode.size - 1].key, sib.prev);
                write(&tn, offset);
                write(&newNode, newPos);
                write(&sib, newNode.succ);
                if (comp(key, tn.index[tn.size - 1].key))
                    insertNewIndex(newNode.prev, key, child);
                else
                    insertNewIndex(tn.succ, key, child);
            }
        }
        else{
            TreeNode newNode;
            createTreeNode(offset, &tn, &newNode);
            int newPos = tn.succ;
            Key old = tn.index[tn.size - 1].key;
            copyIndex(end(tn) - L / 2, end(tn), begin(newNode));
            tn.size -= L / 2;
            newNode.size = L / 2;
            if (tn.parent != 0){
                updateChildIndex(tn.parent, old, tn.index[tn.size - 1].key);
                insertNewIndex(tn.parent, newNode.index[newNode.size - 1].key, tn.succ);
                write(&tn, offset);
                write(&newNode, newPos);
                if (comp(key, tn.index[tn.size - 1].key))
                    insertNewIndex(newNode.prev, key, child);
                else
                    insertNewIndex(tn.succ, key, child);
            }
            else{
                TreeNode newRoot;
                int rootPos = alloc();
                newRoot.prev = newRoot.succ = newRoot.parent = 0;
                newRoot.size = 2;
                newRoot.index[0].key = tn.index[tn.size - 1].key;
                newRoot.index[0].child = newNode.prev;
                newRoot.index[1].key = newNode.index[newNode.size - 1].key;
                newRoot.index[1].child = newPos;
                write(&newRoot, rootPos);
                root = rootPos;
                ++height;
                if (comp(key, tn.index[tn.size - 1].key))
                    insertNewIndex(newNode.prev, key, child);
                else
                    insertNewIndex(tn.succ, key, child);
            }
        }
    }

public:
    BPlusTree(bool CLEAR = false, const char *PATH = WRITE_PATH) {
        strcpy(path, PATH);
        if (CLEAR) {
            openFile("w+");
            closeFile();
        }
        openFile();
        TreeNode rt;
        alloc();
        root = alloc();
        LeafNode ln;
        ln.parent = root;
        rt.index[0].child = alloc();
        write(&rt, root);
        write(&ln, rt.index[0].child);
        height = 2;
    }

    std::pair<Val, bool> search(const Key &key){
        off_t pos = findKey(key, true);
        LeafNode ln;
        read(&ln, pos);
        Record *rc = binarySearchRecord(ln, key);
        if (rc != end(ln) && rc->key == key)
            return std::make_pair(rc->value, true);
        else
            return std::make_pair(Val(), false);
    }

    void insert(const Key &key, const Val &value){
        off_t childPos = findKey(key, true);
        LeafNode ln;
        read(&ln, childPos);
        if (ln.size != L){
            int i;
            for (i = ln.size; i > 0; --i){
                if (!comp(key, ln.record[i - 1].key))
                    break;
                ln.record[i] = ln.record[i - 1];
            }
            ln.record[i].key = key;
            ln.record[i].value = value;
            if (i == ln.size){
                updateChildIndex(ln.parent, ln.record[i - 1].key, key);
            }
            ++ln.size;
            write(&ln, childPos);
            ++_size;
        }
        else if (ln.succ != 0){
            LeafNode sib;
            read(&sib, ln.succ);
            if (sib.size != L){
                copyBackRecord(begin(sib), end(sib), end(sib) + 1);
                copyRecord(end(ln) - 1, end(ln), begin(sib));
                ++sib.size;
                --ln.size;
                write(&sib, ln.succ);
                int i = ln.size;
                for (i = ln.size; i > 0; --i) {
                    if (!comp(key, ln.record[i - 1].key))
                        break;
                    ln.record[i] = ln.record[i - 1];
                }
                ln.record[i].key = key;
                ln.record[i].value = value;
                if (i == ln.size) {
                    updateChildIndex(ln.parent, ln.record[i - 1].key, key);
                }
                else {
                    updateChildIndex(ln.parent, sib.record[0].key, ln.record[ln.size].key);
                }
                ++ln.size;
                write(&ln, childPos);
                ++_size;
            }
            else{
                LeafNode newNode;
                createLeafNode(sib.prev, &ln, &newNode);
                int newPos = ln.succ;
                read(&sib, newNode.succ);
                Key old = ln.record[ln.size - 1].key;
                copyRecord(end(ln) - L / 3, end(ln), begin(newNode));
                ln.size -= L / 3;
                newNode.size += L / 3;
                copyRecord(begin(sib), begin(sib) + L / 3, end(newNode));
                sib.size -= L / 3;
                newNode.size += L / 3;
                copyRecord(begin(sib) + L / 3, begin(sib) + L, begin(sib));
                updateChildIndex(ln.parent, old, ln.record[ln.size - 1].key);
                insertNewIndex(ln.parent, newNode.record[newNode.size - 1].key, sib.prev);
                write(&ln, childPos);
                write(&newNode, newPos);
                write(&sib, newNode.succ);
                insert(key, value);
            }
        }
        else{
            LeafNode newNode;
            createLeafNode(childPos, &ln, &newNode);
            int newPos = ln.succ;
            Key old = ln.record[ln.size - 1].key;
            copyRecord(end(ln) - L / 2, end(ln), begin(newNode));
            ln.size -= L / 2;
            newNode.size = L / 2;
            updateChildIndex(ln.parent, old, ln.record[ln.size - 1].key);
            insertNewIndex(ln.parent, newNode.record[newNode.size - 1].key, ln.succ);
            write(&ln, childPos);
            write(&newNode, newPos);
            insert(key, value);
        }
    }

    void erase(const Key &key){
        off_t pos = findKey(key, true);
        if (pos == 0)
            return;
        LeafNode ln;
        read(&ln, pos);
        int l = 0, r = ln.size - 1, mid;
        while (l < r){
            mid = (l + r) >> 1;
            if (comp(ln.record[mid].key, key))
                l = mid + 1;
            else
                r = mid;
        }
        if (l == ln.size - 1) {
            --ln.size;
            if (ln.size != 0) {
                updateChildIndex(ln.parent, ln.record[l].key, ln.record[l - 1].key);
            }
            else {
                removeIndex(ln.parent, ln.record[l].key);
            }
        }
        else {
            copyRecord(begin(ln) + l + 1, end(ln), begin(ln) + l);
            --ln.size;
            if (ln.succ != 0) {
                LeafNode sib;
                read(&sib, ln.succ);
                if (ln.size + sib.size <= L)
                    mergeLeaf(&ln, &sib);
            }
        }
        write(&ln, pos);
        --_size;
    }

    void update(const Key &key, const Val &value){
        off_t pos = findKey(key);
        if (pos == 0)
            return;
        LeafNode ln;
        read(&ln, pos);
        int l = 0, r = ln.size - 1, mid;
        while (l < r){
            mid = (l + r) >> 1;
            if (comp(key, ln.record[mid].key))
                l = mid + 1;
            else
                r = mid;
        }
        ln.record[l].value = value;
        write(&ln, pos);
    }
    
    size_t size(){
        return _size; 
    }

    bool empty(){
        return (_size == 0);
    }
};

}

#endif //INC_2018DATASTRUCTUREBIGWORK_BPLUSTREE_H
