// This program will encode and decode files using Huffman's algorithm, which relies on binary trees.

#include "encoding.h"
#include "map.h"
#include "hashmap.h"
#include "pqueue.h"
#include "bitstream.h"
#include "filelib.h"
#include "strlib.h"
using namespace std;

void getCharPopulation(istream& input, Map<int, int>& charPopulation);
void buildEncodingTreeHelper(Map<int, int>& charPopulation, HuffmanNode*& encodingTree);
void getDataCodeDict(HuffmanNode* encodingTree, HashMap<int, string>& dataCodeDict, string& temp);
int decodeDataHelper(ibitstream& input, HuffmanNode* encodingTree);

// Record the character's population and build the encoding tree.
HuffmanNode* buildEncodingTree(istream& input) {
    // Construct frequency map.
    Map<int, int> charPopulation;
    getCharPopulation(input, charPopulation);
    HuffmanNode* encodingTree;
    buildEncodingTreeHelper(charPopulation, encodingTree);
    return encodingTree;
}

// Construct frequency map with key being character's index and value being the population.
void getCharPopulation(istream& input, Map<int, int>& charPopulation) {
    int charIndex = input.get();
    while (charIndex != -1) {
        charPopulation[charIndex]++;
        charIndex = input.get();
    }
    // Add the end of file constant.
    charPopulation[PSEUDO_EOF]++;
    rewindStream(input);
}

// Construct the encoding tree by putting the character's population into a priority queue,
// then combine the first 2 elements of the PQ and dequeue and enqueue again.
void buildEncodingTreeHelper(Map<int, int>& charPopulation, HuffmanNode*& encodingTree) {
    // Construct priority queue.
    PriorityQueue<HuffmanNode*> huffPQ;
    for (int key: charPopulation) {
        HuffmanNode* tempNode = new HuffmanNode(key, charPopulation[key]);
        huffPQ.enqueue(tempNode, charPopulation[key]);
    }
    // Construct encoding tree using dequeueing from the priority queue until the queue has size 1.
    while (huffPQ.size() > 1) {
        int tempPriority = huffPQ.peekPriority();
        HuffmanNode* child1 = huffPQ.dequeue();
        tempPriority += huffPQ.peekPriority();
        HuffmanNode* child2 = huffPQ.dequeue();
        HuffmanNode* parent = new HuffmanNode(NOT_A_CHAR, tempPriority, child1, child2);
        huffPQ.enqueue(parent, tempPriority);
    }
    encodingTree = huffPQ.dequeue();
}

// Encode the data into binary numbers and write those integers into output.
void encodeData(istream& input, HuffmanNode* encodingTree, obitstream& output) {
    HashMap<int, string> dataCodeDict;
    string dataCode = "";
    getDataCodeDict(encodingTree, dataCodeDict, dataCode);
    //Read the file character by character, again and for each character find
    //the map key value in dataCodeDict, and change it to bit.
    int charIndex = input.get();
    bool endOfFile = (charIndex == -1);
    while (charIndex != -1 || ! endOfFile) {
        // Encode once more for the end of file index.
        if (charIndex == -1) {
            endOfFile = true;
            charIndex = PSEUDO_EOF;
        }
        string currentCode = dataCodeDict[charIndex];
        for (int i = 0; i < currentCode.size(); i++) {
            // (currentCode[i] - '0') is for changing character into integer based on ASCII index.
            output.writeBit(currentCode[i] - '0');
        }
        charIndex = input.get();
    }
}

// Traversing the tree to get the leaf's information and put that into the data Code Dictionary.
void getDataCodeDict(HuffmanNode* encodingTree, HashMap<int, string>& dataCodeDict, string& dataCode) {
    if (encodingTree->zero == nullptr && encodingTree->one == nullptr) {
        dataCodeDict.put(encodingTree->character, dataCode);
    } else {
        dataCode = dataCode + "0";
        getDataCodeDict(encodingTree->zero, dataCodeDict, dataCode);
        // Backtrack to get to another leaf.
        dataCode = dataCode.substr(0, dataCode.length() - 1);
        dataCode = dataCode + "1";
        getDataCodeDict(encodingTree->one, dataCodeDict, dataCode);
        // Backtrack to get to another leaf.
        dataCode = dataCode.substr(0, dataCode.length() - 1);
    }
}

// Decode data from the bitstream input and output it to the ostream for later use.
void decodeData(ibitstream& input, HuffmanNode* encodingTree, ostream& output) {
    int currentChar = decodeDataHelper(input, encodingTree);
    while (currentChar != PSEUDO_EOF) {
        output << char(currentChar);
        currentChar = decodeDataHelper(input, encodingTree);
    }
}

// Traversing the encoding tree by going through the input 0s and 1s one by one.
// Return the character as integer when it reaches the leaf of the tree.
int decodeDataHelper(ibitstream& input, HuffmanNode* encodingTree) {
    if (encodingTree->isLeaf()) {
        return encodingTree->character;
    } else {
        int currentDigit = input.readBit();
        // for the case where the input is empty.
        if (currentDigit == -1) {
            return PSEUDO_EOF;
        }
        if (currentDigit == 0){
            return decodeDataHelper(input, encodingTree->zero);
        } else {
            return decodeDataHelper(input, encodingTree->one);
        }
    }
}

// Compress file by writing the header and then encoding the data.
void compress(istream& input, obitstream& output) {
    // Write header to compressed file.
    Map <int, int> charPopulation;
    getCharPopulation(input, charPopulation);
    output << charPopulation;
    // Build encoding tree.
    HuffmanNode* encodingTree = buildEncodingTree(input);
    encodeData(input, encodingTree, output);
}

// Uncompress file by taking out the header and then decode the data.
void uncompress(ibitstream& input, ostream& output) {
    // Read header from compressed file.
    Map <int, int> charPopulation;
    input >> charPopulation;
    // Uncompress data after header if the file is not empty.
    if (charPopulation.size() > 1) {
        HuffmanNode* encodingTree;
        buildEncodingTreeHelper(charPopulation, encodingTree);
        decodeData(input, encodingTree, output);
    }
}

/* Delete the leaf of the tree one by one. Then delete sub-root, until the main
 * root is deleted and tree is free.*/
void freeTree(HuffmanNode* node) {
    if (node != nullptr){
        freeTree(node->zero);
        freeTree(node->one);
        delete node;
        node = nullptr;
    }
}

