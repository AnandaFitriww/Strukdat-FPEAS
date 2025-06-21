#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>
#include <stack>
#include <map>
#include <set>
#include <algorithm>
#include <random>
#include <ctime>
#include <thread>
#include <chrono>
#include <unistd.h>

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

GenreNode* rootGenre = new GenreNode("Library");
map<string, Book*> isbnMap;
map<string, Book*> titleMap;
map<string, GenreNode*> genreMap;
map<string, int> genreScore, authorScore;
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
            genreMap[genre] = node;
        }
        node->addBook(book);
    }
    cout << "Books loaded from " << filename << "\n";
}

void showAllBooks() {
    cout << "\nAll Books in Library (sorted by ISBN):\n";
    vector<pair<string, Book*>> sortedBooks(isbnMap.begin(), isbnMap.end());
    sort(sortedBooks.begin(), sortedBooks.end());
    for (auto& pair : sortedBooks) {
        Book* b = pair.second;
        cout << b->isbn << " - " << b->title << " by " << b->author
             << " [" << (b->isAvailable ? "Available" : "Borrowed") << "]\n";
    }
}

Book* getBookInput(string action) {
    cout << action << " by (1) ISBN or (2) Title? ";
    int opt; cin >> opt; cin.ignore();
    string input;
    cout << "Enter value: "; getline(cin, input);
    if (opt == 1 && isbnMap.count(input)) return isbnMap[input];
    if (opt == 2 && titleMap.count(input)) return titleMap[input];
    return nullptr;
}

void borrowBook() {
    Book* book = getBookInput("Borrow");
    if (!book) { cout << "Book not found.\n"; return; }
    if (!book->isAvailable) { cout << "Book is already borrowed.\n"; return; }
    book->isAvailable = false;
    borrowQueue.push(book);
    actionHistory.push({ BORROW, book });
    genreScore[book->genre]++;
    authorScore[book->author]++;
    cout << "Book borrowed: " << book->title << "\n";
}

void returnBook() {
    Book* book = getBookInput("Return");
    if (!book) { cout << "Book not found.\n"; return; }
    if (book->isAvailable) { cout << "Book is already available.\n"; return; }
    book->isAvailable = true;
    returnQueue.push(book);
    actionHistory.push({ RETURN, book });
    cout << "Book returned: " << book->title << "\n";
}

void undoLastAction() {
    if (actionHistory.empty()) {
        cout << "No actions to undo.\n"; return;
    }
    Action last = actionHistory.top(); actionHistory.pop();
    if (last.type == BORROW) {
        last.book->isAvailable = true;
        cout << "Undo: Book returned (" << last.book->title << ")\n";
    } else {
        last.book->isAvailable = false;
        cout << "Undo: Book borrowed again (" << last.book->title << ")\n";
    }
}

void searchBook() {
    Book* book = getBookInput("Search");
    if (!book) { cout << "Book not found.\n"; return; }
    cout << book->title << " by " << book->author
         << " (ISBN: " << book->isbn << ") ["
         << (book->isAvailable ? "Available" : "Borrowed") << "]\n";
}

void showBooksByGenre() {
    cout << "\n1. List available genres\n2. View books by genre\nChoice: ";
    int opt; cin >> opt; cin.ignore();
    if (opt == 1) {
        cout << "Available Genres:\n";
        for (auto& pair : genreMap)
            cout << "- " << pair.first << "\n";
    } else {
        cout << "Enter genre: ";
        string g; getline(cin, g);
        GenreNode* node = rootGenre->findGenre(g);
        if (node) node->displayBooks();
        else cout << "Genre not found.\n";
    }
}

void recommendBooks() {
    if (genreScore.empty() && authorScore.empty()) {
        // show random 3 books
        cout << "No history found. Showing random recommendations:\n";
        vector<Book*> allBooks;
        for (auto& pair : isbnMap)
            if (pair.second->isAvailable) allBooks.push_back(pair.second);
        shuffle(allBooks.begin(), allBooks.end(), default_random_engine(time(0)));
        for (int i = 0; i < min(3, (int)allBooks.size()); ++i) {
            Book* b = allBooks[i];
            cout << "- " << b->title << " by " << b->author << " (" << b->isbn << ")\n";
        }
        return;
    }

    map<Book*, int> score;
    for (auto& pair : isbnMap) {
        Book* b = pair.second;
        if (!b->isAvailable) continue;
        score[b] = genreScore[b->genre] + authorScore[b->author];
    }
    vector<pair<int, Book*>> ranked;
    for (auto& pair : score)
        ranked.push_back({ pair.second, pair.first });
    sort(ranked.rbegin(), ranked.rend());
    cout << "Recommended books:\n";
    for (int i = 0; i < min(3, (int)ranked.size()); ++i) {
        Book* b = ranked[i].second;
        cout << "- " << b->title << " by " << b->author << " (" << b->isbn << ")\n";
    }
}

void menu() {
    int choice;
    do {
        cout << "\n+----+ Perpus Cihuyy +----+\n";
        cout << "===========================\n";
        cout << "|  1. Show All Books      |\n";
        cout << "|  2. Search Book         |\n";
        cout << "|  3. Show Book by Genre  |\n";
        cout << "|  4. Borrow Book         |\n";
        cout << "|  5. Return Book         |\n";
        cout << "|  6. Undo Last Action    |\n";
        cout << "|  7. Recommend Books     |\n";
        cout << "|  8. Exit                |\n";
        cout << "===========================\n";
        cout << "Choice: ";
        cin >> choice;

        switch (choice) {
            case 1: showAllBooks(); break;
            case 2: searchBook(); break;
            case 3: showBooksByGenre(); break;
            case 4: borrowBook(); break;
            case 5: returnBook(); break;
            case 6: undoLastAction(); break;
            case 7: recommendBooks(); break;
            case 8: cout << "Exiting...\n";
                    sleep(1);
                    cout << "Thank You シ\n"; break;
            default: cout << "Invalid choice.\n";
        }
    } while (choice != 8);
}

int main() {
    loadBooksFromFile("books.txt");
    menu();
    return 0;
}
