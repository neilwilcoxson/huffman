/*
 * Author: Neil Wilcoxson
 * Assignment Title: Project 4 - Huffman Encoding
 * Due Date: 3/21/2018
 * Date Created: 3/1/2018
 * Date Last Modified: 3/1/2018
 */

#include <iostream>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <queue>

using namespace std;


//TODO function header comments
/*
 * name
 *
 * description:
 * return:
 * precondition:
 * postcondition:
 */

struct TreeNode{
    bool isChar, rightChild;
    char letter;
    int frequency;
    TreeNode *left, *right, *parent;

    explicit TreeNode(bool ic = true, char c = 'a'){
        this->isChar = ic;
        this->letter = c;
        this->frequency = 0;
        this->left = this->right = this->parent = nullptr;
        this->rightChild = false;
    }
    ~TreeNode(){
        delete left;
        delete right;
    }

    bool operator<(TreeNode& that){
        return this->frequency < that.frequency;
    }
};

bool TreeNodePtrCompare(TreeNode* a, TreeNode* b){
    return a->frequency < b->frequency;
}
TreeNode* queueToTree(queue<TreeNode*>& q);

class HuffmanTree{
protected:
    TreeNode *root, **array, **refArray;
    int numChar;
    int id;
public:
    HuffmanTree();
    ~HuffmanTree();
    bool verify(int id);
    bool buildTree(ifstream& f);
    void encode(ifstream& in, ofstream& out);
    void decode(ifstream& in, ofstream& out);
    friend ostream& operator<<(ostream &os, const HuffmanTree& ht);
    friend istream& operator>>(istream &is, HuffmanTree& ht);
};

int main(int argc, char** argv){
    if(argc < 4){
        if(argc >= 2 && strcmp(argv[1], "--help") == 0){
            cerr << "To compress use -huff\n"
                 << "To decompress use -unhuff\n\n";
        }
        cerr << "Usage: huffman -huff <source> <destination>\n"
             << "Usage: huffman -unhuff <source> <destination>\n\n"
             << "NOTE: If destination exists, it will be overwritten!" << endl;
        exit(1);
    }

    ifstream src;
    ofstream dest;

    src.open(argv[2]);

    if(!src.good()){
        cerr << "File error: Could not open source file: " << argv[2] << endl;
        exit(2);
    }

    dest.open(argv[3]);

    if(!dest.good()){
        cerr << "File error: Could not write to destination file: "
             << argv[3] << endl;
        src.close();
        exit(2);
    }

    HuffmanTree h;

    if(strcmp(argv[1],"-huff") == 0){
        if(h.buildTree(src)){
            dest << h;
            h.encode(src, dest);

        }else{
            cerr << "File cannot be compressed" << endl;
            src.close();
            dest.close();
            exit(3);
        }
    }else if(strcmp(argv[1],"-unhuff") == 0){
        int key;
        src.read((char*)&key, sizeof(int));

        if(h.verify(key)){
            src >> h;
            h.decode(src, dest);
        }else{
            cerr << "File was not compressed by this program" << endl;
            src.close();
            dest.close();
            exit(4);
        }
    }else{
        cerr << "Unrecognized command line option" << endl;
        src.close();
        dest.close();
        exit(5);
    }

    src.close();
    dest.close();

    return 0;
}

HuffmanTree::HuffmanTree(){
    this->root = nullptr;
    this->numChar = 0;
    this->id = 0xfeedc4c5;
    this->array = this->refArray = nullptr;
}
HuffmanTree::~HuffmanTree(){
    delete root;
    delete [] array;
}
bool HuffmanTree::verify(int id){
    return id == this->id;
}
bool HuffmanTree::buildTree(ifstream& f){
    unsigned long charCount = 0, bytesRequired = 8;
    char current;

    if(!array){
        array = new TreeNode*[256];
    }
    if(!refArray){
        refArray = new TreeNode*[256];
    }

    for(int i = 0; i < 256; i++){
        array[i] = new TreeNode(true, i);
        refArray[i] = array[i];
    }

    f.seekg(ios::beg);

    array[255]->frequency = 1;

    while(f.read(&current,1)){
        array[current]->frequency++;
        charCount++;
    }

    sort(refArray,refArray+256,TreeNodePtrCompare);

    queue<TreeNode*> q;

    for(int i = 0; i < 256; i++){
        if(refArray[i]->frequency > 0){
            q.push(refArray[i]);
            bytesRequired += 5;
            numChar++;
        }else{
            delete refArray[i];
        }
    }

    this->root = queueToTree(q);

    delete [] refArray;

    f.clear();
    f.seekg(ios::beg);

    //TODO use frequency instead of reading file again
    int bits = 0;

    while(f.read(&current,1)){
        TreeNode* currentNode = array[current];

        while(currentNode != this->root){
            bits++;

            if(bits >= 8){
                bytesRequired++;
                bits = 0;
            }

            currentNode = currentNode->parent;
        }
        if(bytesRequired >= charCount){
            return false;
        }
    }
    if(bits){
        bytesRequired++;
    }

    return bytesRequired < charCount;
}

void HuffmanTree::encode(ifstream& in, ofstream& out){
    char c, buffer = 0;
    int currentPattern = 0, pBits = 0, bBits = 0, extra = 1;
    TreeNode* currentNode;

    in.clear();
    in.seekg(ios::beg);

    while(in.read(&c,1) || extra--){
        currentNode = array[extra ? c : 255];

        while(currentNode != this->root){
            currentPattern <<= 1;
            currentPattern |= currentNode->rightChild ? 1 : 0;
            pBits++;

            currentNode = currentNode->parent;
        }

        while(pBits > 0){
            buffer <<= 1;
            buffer |= currentPattern & 1;
            currentPattern >>= 1;
            pBits--;

            bBits++;

            if(bBits == 8){
                out.write(&buffer,1);
                buffer = 0;
                bBits = 0;
            }
        }
    }

    if(bBits){
        buffer <<= 8 - bBits;
        out.write(&buffer,1);
    }
}

void HuffmanTree::decode(ifstream& in, ofstream& out){
    char buffer;
    TreeNode* current = this->root;

    in.clear();
    while(in.read(&buffer,1)){
        for(int i = 7; i >= 0; i--){
            int temp = (buffer >> i) & 1;
            current = temp ? current->right : current->left;

            if(current->isChar){
                if(current->letter == -1){
                    return;
                }else{
                    out.write(&current->letter, 1);
                    current = this->root;
                }
            }
        }
    }
}

ostream& operator<<(ostream &os, const HuffmanTree& ht){
    os.write((char*)&ht.id, sizeof(int));
    os.write((char*)&ht.numChar, sizeof(int));
    for(int i = 256-ht.numChar; i < 256; i++){
        if(ht.refArray[i] && ht.refArray[i]->frequency > 0){
            os.write(&ht.refArray[i]->letter,1);
            os.write((char*)&ht.refArray[i]->frequency, sizeof(int));
        }
    }
    return os;
}
istream& operator>>(istream &is, HuffmanTree& ht){
    char c;
    int f;

    is.read((char*)&ht.numChar, sizeof(int));

    if(!ht.array){
        ht.array = new TreeNode*[ht.numChar];
    }

    queue<TreeNode*> q;

    for(int i = 0; i < ht.numChar; i++){
        is.read(&c,1);
        is.read((char*)&f, sizeof(int));

        ht.array[i] = new TreeNode(true, c);
        ht.array[i]->frequency = f;

        q.push(ht.array[i]);
    }

    ht.root = queueToTree(q);

    return is;
}

TreeNode* queueToTree(queue<TreeNode*>& q){
    TreeNode* root = nullptr;

    while(!q.empty()){
        TreeNode* temp = new TreeNode(false);
        temp->left = q.front();
        temp->left->parent = temp;
        temp->frequency = temp->left->frequency;
        q.pop();

        if(!q.empty()){
            temp->right = q.front();
            temp->right->parent = temp;
            temp->frequency += temp->right->frequency;
            temp->right->rightChild = true;
            q.pop();

            q.push(temp);
        }else{
            root = temp->left;
            root->parent = root;
            temp->left = nullptr;
            delete temp;
        }
    }

    return root;
}