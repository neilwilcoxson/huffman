/*
 * Author: Neil Wilcoxson
 * Assignment Title: Project 4 - Huffman Encoding
 * Due Date: 3/21/2018
 * Date Created: 3/1/2018
 * Date Last Modified: 3/17/2018
 *
 * This program uses huffman encoding to compress and decompress files
 * (primarily text).  File are given as command line arguments. See help
 * message for success/error codes.
 */

#include <iostream>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <queue>

using namespace std;

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
    int id;                 //used for verification
    unsigned char numChar;  //number of unique characters
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
    bool buildTree(fstream& f);

    /*
     * HuffmanTree::encode
     *
     * description: compresses contents of input file to output file
     * return: void
     * precondition: input/output files are open, tree written to output file
     * postcondition: compressed data written to output file
     */
    void encode(fstream& in, fstream& out);

    /*
     * HuffmanTree::decode
     *
     * description: decompresses contents on input file to output file
     * return: void
     * precondition: both files are open, input file marker at data start
     * postcondition: decompressed data written to output file
     */
    void decode(fstream& in, fstream& out);

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

//used to exit smoothly anytime the program can't continue
class unsuccessful : public exception{
public:
    string msg;  //error message: can be printed in catch block
    int code;    //error code: can be returned in catch block

    unsuccessful(string s, int i){
        msg = move(s);
        code = i;
    }
};

int main(int argc, char** argv){
    //incorrect number of arguments
    if(argc < 4){
        if(argc >= 2 && strcmp(argv[1], "--help") == 0){
            cerr << "To compress use -huff\n"
                 << "To decompress use -unhuff\n\n"
                 << "Success/Error Codes:\n"
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
    fstream src, dest;

    try{
        src.open(argv[2], ios::in | ios::binary);

        if(!src.good()){
            string error = "File error: Could not open source file: ";
            error += argv[2];
            throw unsuccessful(error,2);
        }

        dest.open(argv[3], ios::out | ios::binary);

        if(!dest.good()){
            string error = "File error: Could not write to destination file: ";
            error += argv[3];
            throw unsuccessful(error,2);
        }

        HuffmanTree h;

        //compress
        if(strcmp(argv[1], "-huff") == 0){
            if(h.buildTree(src)){
                dest << h;
                h.encode(src, dest);

            }else{
                throw unsuccessful("File cannot be compressed", 3);
            }
        }

        //decompress
        else if(strcmp(argv[1], "-unhuff") == 0){
            int key;
            src.read((char*)&key, sizeof(int));

            if(h.verify(key)){
                src >> h;
                h.decode(src, dest);
            }else{
                throw unsuccessful("File was not compressed by this program", 3);
            }
        }
        //unrecognized
        else{
            throw unsuccessful("Unrecognized command line option", 1);
        }

    }catch(unsuccessful& e){
        cerr << e.msg << endl;
        src.close();
        dest.close();
        return e.code;
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

bool HuffmanTree::buildTree(fstream& f){
    unsigned long charCount = 0, bytesRequired = 5;
    int bits = 0;
    char c;

    if(!array){
        array = new TreeNode*[256];
    }
    if(!refArray){
        refArray = new TreeNode*[256];
    }

    for(int i = 0; i < 256; i++){
        array[i] = new TreeNode(true, (char)i);
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

            //header: 1 byte for character
            bytesRequired++;

            //number of unique characters in tree
            numChar++;
        }else{
            //not used in the tree
            delete refArray[i];
        }
    }

    this->root = queueToTree(q);

    //determine how many steps to the root --> how many bytes
    for(int i = 256-this->numChar; i < 256; i++){
        //data: number of bytes varies
        TreeNode* currentNode = refArray[i];

        while(currentNode != this->root){
            bits += refArray[i]->frequency;

            while(bits >= 8){
                bytesRequired++;
                bits -= 8;
            }

            currentNode = currentNode->parent;
        }
    }

    //if there are still bits remaining, we need an extra byte
    if(bits){
        bytesRequired++;
    }

    return bytesRequired < charCount;
}

void HuffmanTree::encode(fstream& in, fstream& out){
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

void HuffmanTree::decode(fstream& in, fstream& out){
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
    os.write((char*)&ht.numChar, 1);

    for(int i = 256-ht.numChar; i < 256; i++){
        if(ht.refArray[i] && ht.refArray[i]->frequency > 0){
            os.write(&ht.refArray[i]->letter,1);
        }
    }
    return os;
}

istream& operator>>(istream &is, HuffmanTree& ht){
    char c;

    is.read((char*)&ht.numChar, 1);

    if(!ht.refArray){
        ht.refArray = new TreeNode*[ht.numChar];
    }

    queue<TreeNode*> q;

    //read characters and add to queue
    for(int i = 0; i < ht.numChar; i++){
        is.read(&c,1);

        ht.refArray[i] = new TreeNode(true, c);

        q.push(ht.refArray[i]);
    }

    ht.root = queueToTree(q);

    return is;
}

TreeNode* queueToTree(queue<TreeNode*>& q){
    TreeNode *root = nullptr, *temp;

    while(!q.empty()){
        temp = new TreeNode(false);

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