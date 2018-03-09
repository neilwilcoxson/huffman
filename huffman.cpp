/*
 * Author: Neil Wilcoxson
 * Assignment Title: Project 4 - Huffman Encoding
 * Due Date: 3/21/2018
 * Date Created: 3/1/2018
 * Date Last Modified: 3/8/2018
 */

#include <iostream>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <queue>

using namespace std;

//TODO what if the file is too large

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

    //WARNING: Destructor deletes all children nodes
    ~TreeNode(){
        delete left;
        delete right;
    }
};

/*
 * TreeNodePtrCompare
 *
 * description: compares TreeNode* based on frequency for sorting
 * return: bool
 * precondition: pointers have been assigned
 * postcondition: true/false (a < b) returned
 */
bool TreeNodePtrCompare(TreeNode* a, TreeNode* b){
    return a->frequency < b->frequency;
}

/*
 * queueToTree
 *
 * description: builds a HuffmanTree based on leaf nodes (letters) in queue
 * return: TreeNode*
 * precondition: all letters are queued with lowest frequency at front
 * postcondition: root node of new tree is returned
 */
TreeNode* queueToTree(queue<TreeNode*>& q);

class HuffmanTree{
protected:
    TreeNode *root,         //root node of the tree
            **array,        //sorted by numerical value (0-255)
            **refArray;     //sorted by frequency
    int numChar,            //number of unique characters
        id;                 //used for verification
public:
    HuffmanTree();
    ~HuffmanTree();

    /*
     * HuffmanTree::verify
     *
     * description: verifies tree read from file was created by this program
     * return: bool
     * precondition: id read from file and stored in int
     * postcondition: flag representing whether tree is compatible
     */
    bool verify(int id);

    /*
     * HuffmanTree::buildTree
     *
     * description: builds a tree in memory from an uncompressed file
     * return: bool
     * precondition: file is open
     * postcondition: flag representing whether compresses will work
     */
    bool buildTree(ifstream& f);

    /*
     * HuffmanTree::encode
     *
     * description: compresses contents of input file to output file
     * return: void
     * precondition: input/output files are open, tree written to output file
     * postcondition: compressed data written to output file
     */
    void encode(ifstream& in, ofstream& out);

    /*
     * HuffmanTree::decode
     *
     * description: decompresses contents on input file to output file
     * return: void
     * precondition: both files are open, input file marker at data start
     * postcondition: decompressed data written to output file
     */
    void decode(ifstream& in, ofstream& out);

    /*
     * operator<<
     *
     * description: outputs tree data, including id
     * return: ostream&
     * precondition: tree has been built
     * postcondition: tree data written to output stream
     */
    friend ostream& operator<<(ostream &os, const HuffmanTree& ht);

    /*
     * operator>>
     *
     * description: reads in tree data from input stream
     * return: istream&
     * precondition: id has been verified, marker at tree data
     * postcondition: tree is reassembled
     */
    friend istream& operator>>(istream &is, HuffmanTree& ht);
};

int main(int argc, char** argv){
    //incorrect number of arguments
    if(argc < 4){
        if(argc >= 2 && strcmp(argv[1], "--help") == 0){
            cerr << "To compress use -huff\n"
                 << "To decompress use -unhuff\n\n"
                 << "Error Codes:\n"
                 << "0 - operation completed successfully\n"
                 << "1 - incorrect number of, unrecognized arguments\n"
                 << "2 - file read/write error\n"
                 << "3 - unable to compress/decompress\n\n";
        }
        cerr << "Usage: huffman -huff <source> <destination>\n"
             << "Usage: huffman -unhuff <source> <destination>\n\n"
             << "NOTE: If destination exists, it will be overwritten!" << endl;
        return 1;
    }

    ifstream src;
    ofstream dest;

    src.open(argv[2]);

    if(!src.good()){
        cerr << "File error: Could not open source file: " << argv[2] << endl;
        return 2;
    }

    dest.open(argv[3]);

    if(!dest.good()){
        cerr << "File error: Could not write to destination file: "
             << argv[3] << endl;
        src.close();
        return 2;
    }

    HuffmanTree h;

    //compress
    if(strcmp(argv[1],"-huff") == 0){
        if(h.buildTree(src)){
            dest << h;
            h.encode(src, dest);

        }else{
            cerr << "File cannot be compressed" << endl;
            src.close();
            dest.close();
            return 3;
        }
    }

    //decompress
    else if(strcmp(argv[1],"-unhuff") == 0){
        int key;
        src.read((char*)&key, sizeof(int));

        if(h.verify(key)){
            src >> h;
            h.decode(src, dest);
        }else{
            cerr << "File was not compressed by this program" << endl;
            src.close();
            dest.close();
            return 3;
        }
    }

    //unrecognized
    else{
        cerr << "Unrecognized command line option" << endl;
        src.close();
        dest.close();
        return 1;
    }

    src.close();
    dest.close();

    return 0;
}

/*** Definitions ***/

HuffmanTree::HuffmanTree(){
    this->root = nullptr;
    this->numChar = 0;
    this->id = 0xfeedc4c5;
    this->array = this->refArray = nullptr;
}
HuffmanTree::~HuffmanTree(){
    delete root;
    delete [] array;
    delete [] refArray;
}
bool HuffmanTree::verify(int id){
    return id == this->id;
}
bool HuffmanTree::buildTree(ifstream& f){
    unsigned long charCount = 0, bytesRequired = 8;
    int bits = 0;
    char c;

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

    //255 (-1 signed) is used as the EOF character
    array[255]->frequency = 1;

    f.seekg(ios::beg);

    //count frequency of each character, and how many characters total in file
    while(f.read(&c,1)){
        array[c]->frequency++;
        charCount++;
    }

    //sort reference array based on frequency
    sort(refArray,refArray+256,TreeNodePtrCompare);

    queue<TreeNode*> q;

    for(int i = 0; i < 256; i++){
        if(refArray[i]->frequency > 0){
            //only push characters that actually appear
            q.push(refArray[i]);

            //header: 1 byte for character + 4 bytes for frequency
            bytesRequired += 5;

            //data: number of bytes varies
            TreeNode* currentNode = array[i];

            //determine how many steps to the root --> how many bytes
            while(currentNode != this->root){
                bits++;

                if(bits >= 8){
                    bytesRequired++;
                    bits = 0;
                }

                currentNode = currentNode->parent;
            }

            //number of unique characters in tree
            numChar++;
        }else{
            //not used in the tree
            delete refArray[i];
        }
    }

    //if there are still bits remaining, we need an extra byte
    if(bits){
        bytesRequired++;
    }

    this->root = queueToTree(q);

    return bytesRequired < charCount;
}

void HuffmanTree::encode(ifstream& in, ofstream& out){
    char c, buffer = 0;
    int currentPattern = 0, pBits = 0, bBits = 0, extra = 1;
    TreeNode* currentNode;

    //start at beginning of input file
    in.clear();
    in.seekg(ios::beg);

    //keep reading until we can't, then once more for EOF
    while(in.read(&c,1) || extra--){
        //use c, except for last iteration, use EOF (255)
        currentNode = array[extra ? c : 255];

        //determine pattern to root
        while(currentNode != this->root){
            currentPattern <<= 1;
            currentPattern |= currentNode->rightChild ? 1 : 0;
            pBits++;

            currentNode = currentNode->parent;
        }

        while(pBits > 0){
            //make one space on the right
            buffer <<= 1;

            //set the right bit if it is set in currentPattern
            buffer |= currentPattern & 1;

            //adjust for next iteration
            currentPattern >>= 1;
            pBits--;
            bBits++;

            //once there is a full byte, write to file
            if(bBits == 8){
                out.write(&buffer,1);
                buffer = 0;
                bBits = 0;
            }
        }
    }

    //write one more byte padded with zeroes if necessary
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
            //determine if the bit is set and follow directions
            int temp = (buffer >> i) & 1;
            current = temp ? current->right : current->left;

            if(current->isChar){
                //Is it the EOF character?
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

    if(!ht.refArray){
        ht.refArray = new TreeNode*[ht.numChar];
    }

    queue<TreeNode*> q;

    //read characters and frequencies and add to queue
    for(int i = 0; i < ht.numChar; i++){
        is.read(&c,1);
        is.read((char*)&f, sizeof(int));

        ht.refArray[i] = new TreeNode(true, c);
        ht.refArray[i]->frequency = f;

        q.push(ht.refArray[i]);
    }

    ht.root = queueToTree(q);

    return is;
}

TreeNode* queueToTree(queue<TreeNode*>& q){
    TreeNode* root = nullptr;

    while(!q.empty()){
        TreeNode* temp = new TreeNode(false);

        //put the least frequent item on the left
        temp->left = q.front();
        temp->left->parent = temp;
        temp->frequency = temp->left->frequency;
        q.pop();

        if(!q.empty()){
            //put the next least frequent on the right
            temp->right = q.front();
            temp->right->parent = temp;
            temp->frequency += temp->right->frequency;
            temp->right->rightChild = true;
            q.pop();

            //add to the back of the queue and repeat
            q.push(temp);
        }else{
            //nothing left on queue, cleanup
            root = temp->left;
            root->parent = root;
            temp->left = nullptr;
            delete temp;
        }
    }

    return root;
}