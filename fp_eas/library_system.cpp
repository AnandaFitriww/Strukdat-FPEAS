#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>
#include <stack>
#include <map>
#include <algorithm>

using namespace std;

enum ActionType { BORROW, RETURN };

struct Book {
    string title, isbn, author, genre;
    bool isAvailable;

    Book(string t, string i, string a, string g)
        : title(t), isbn(i), author(a), genre(g), isAvailable(true) {}
};

struct Action {
    ActionType type;
    Book* book;
};

class GenreNode {
public:
    string genre;
    vector<Book*> books;
    vector<GenreNode*> subGenres;

    GenreNode(string g) : genre(g) {}

    void addBook(Book* book) {
        books.push_back(book);
    }

    void displayBooks() {
        for (Book* b : books) {
            cout << "- " << b->title << " (" << b->isbn << ") ["
                 << (b->isAvailable ? "Available" : "Borrowed") << "]\n";
        }
    }

    GenreNode* findGenre(const string& g) {
        if (genre == g) return this;
        for (GenreNode* sub : subGenres) {
            GenreNode* found = sub->findGenre(g);
            if (found) return found;
        }
        return nullptr;
    }

    void addSubGenre(GenreNode* node) {
        subGenres.push_back(node);
    }
};

// Global structures
GenreNode* rootGenre = new GenreNode("Library");
map<string, Book*> isbnMap;
map<string, Book*> titleMap;
queue<Book*> borrowQueue;
queue<Book*> returnQueue;
stack<Action> actionHistory;

void loadBooksFromFile(string filename) {
    ifstream file(filename);
    string line;

    while (getline(file, line)) {
        stringstream ss(line);
        string title, isbn, author, genre;
        getline(ss, title, ',');
        getline(ss, isbn, ',');
        getline(ss, author, ',');
        getline(ss, genre, ',');

        Book* book = new Book(title, isbn, author, genre);
        isbnMap[isbn] = book;
        titleMap[title] = book;

        GenreNode* node = rootGenre->findGenre(genre);
        if (!node) {
            node = new GenreNode(genre);
            rootGenre->addSubGenre(node);
        }
        node->addBook(book);
    }

    cout << "Books loaded from " << filename << "\n";
}

void borrowBook() {
    string isbn;
    cout << "Enter ISBN to borrow: ";
    cin >> isbn;

    if (isbnMap.find(isbn) != isbnMap.end()) {
        Book* book = isbnMap[isbn];
        if (book->isAvailable) {
            book->isAvailable = false;
            borrowQueue.push(book);
            actionHistory.push({ BORROW, book });
            cout << "Book borrowed: " << book->title << "\n";
        } else {
            cout << "Book is already borrowed.\n";
        }
    } else {
        cout << "Book not found.\n";
    }
}

void returnBook() {
    string isbn;
    cout << "Enter ISBN to return: ";
    cin >> isbn;

    if (isbnMap.find(isbn) != isbnMap.end()) {
        Book* book = isbnMap[isbn];
        if (!book->isAvailable) {
            book->isAvailable = true;
            returnQueue.push(book);
            actionHistory.push({ RETURN, book });
            cout << "Book returned: " << book->title << "\n";
        } else {
            cout << "Book is already available.\n";
        }
    } else {
        cout << "Book not found.\n";
    }
}

void undoLastAction() {
    if (actionHistory.empty()) {
        cout << "No actions to undo.\n";
        return;
    }

    Action last = actionHistory.top();
    actionHistory.pop();

    if (last.type == BORROW) {
        last.book->isAvailable = true;
        cout << "Undo: Book returned (" << last.book->title << ")\n";
    } else {
        last.book->isAvailable = false;
        cout << "Undo: Book borrowed again (" << last.book->title << ")\n";
    }
}

void searchBook() {
    string input;
    cout << "Search by (1) ISBN or (2) Title: ";
    int opt; cin >> opt;

    cout << "Enter keyword: ";
    cin.ignore(); getline(cin, input);

    if (opt == 1 && isbnMap.find(input) != isbnMap.end()) {
        Book* b = isbnMap[input];
        cout << b->title << " by " << b->author << " ["
             << (b->isAvailable ? "Available" : "Borrowed") << "]\n";
    } else if (opt == 2 && titleMap.find(input) != titleMap.end()) {
        Book* b = titleMap[input];
        cout << b->title << " by " << b->author << " ["
             << (b->isAvailable ? "Available" : "Borrowed") << "]\n";
    } else {
        cout << "Book not found.\n";
    }
}

void showBooksByGenre() {
    string g;
    cout << "Enter genre to view: ";
    cin.ignore(); getline(cin, g);

    GenreNode* node = rootGenre->findGenre(g);
    if (node) {
        cout << "Books in genre '" << g << "':\n";
        node->displayBooks();
    } else {
        cout << "Genre not found.\n";
    }
}

void recommendBooks() {
    string g;
    cout << "Enter preferred genre: ";
    cin.ignore(); getline(cin, g);

    GenreNode* node = rootGenre->findGenre(g);
    if (node) {
        cout << "Recommended books in genre '" << g << "':\n";
        node->displayBooks();
    } else {
        cout << "No recommendations available.\n";
    }
}

void menu() {
    int choice;
    do {
        cout << "\n--- Library Management ---\n";
        cout << "1. Borrow Book\n";
        cout << "2. Return Book\n";
        cout << "3. Undo Last Action\n";
        cout << "4. Search Book\n";
        cout << "5. Show Books by Genre\n";
        cout << "6. Recommend Books\n";
        cout << "7. Exit\n";
        cout << "Choice: ";
        cin >> choice;

        switch (choice) {
            case 1: borrowBook(); break;
            case 2: returnBook(); break;
            case 3: undoLastAction(); break;
            case 4: searchBook(); break;
            case 5: showBooksByGenre(); break;
            case 6: recommendBooks(); break;
            case 7: cout << "Exiting...\n"; break;
            default: cout << "Invalid choice.\n";
        }

    } while (choice != 7);
}

int main() {
    loadBooksFromFile("books.txt");
    menu();
    return 0;
}
