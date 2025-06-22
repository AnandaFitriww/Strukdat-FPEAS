#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stack>
#include <map>
#include <set>
#include <list>
#include <algorithm>
#include <cctype>

using namespace std;

class Book;
class User;

class Action {
public:
    enum ActionType { BORROW, RETURN };
    ActionType type;
    Book* book;
};

class BookNode {
public:
    Book* book;
    map<BookNode*, int> adjacentNodes;

    BookNode(Book* b) : book(b) {}
};

class BookGraph {
private:
    map<Book*, BookNode*> nodeMap;

public:
    ~BookGraph() {
        for (auto const& pair : nodeMap) {
            delete pair.second;
        }
    }

    void addBook(Book* book) {
        if (nodeMap.find(book) == nodeMap.end()) {
            nodeMap[book] = new BookNode(book);
        }
    }

    void addEdge(Book* book1, Book* book2, int weight) {
        if (book1 == book2 || nodeMap.find(book1) == nodeMap.end() || nodeMap.find(book2) == nodeMap.end()) {
            return;
        }
        BookNode* node1 = nodeMap[book1];
        BookNode* node2 = nodeMap[book2];
        node1->adjacentNodes[node2] += weight;
        node2->adjacentNodes[node1] += weight;
    }

    BookNode* getNode(Book* book) {
        if (nodeMap.count(book)) {
            return nodeMap[book];
        }
        return nullptr;
    }
};

class GenreNode {
public:
    string genre;
    vector<Book*> books;
    vector<GenreNode*> subGenres;

    GenreNode(string g) : genre(g) {}

    ~GenreNode() {
        for (GenreNode* sub : subGenres) {
            delete sub;
        }
    }

    void addBook(Book* book) {
        books.push_back(book);
    }

    GenreNode* findSubGenre(const string& g) {
        for (GenreNode* sub : this->subGenres) {
            if (sub->genre == g) {
                return sub;
            }
        }
        return nullptr;
    }

    void collectBooksRecursively(set<Book*>& bookSet) const {
        for (Book* b : this->books) {
            bookSet.insert(b);
        }
        for (GenreNode* sub : this->subGenres) {
            sub->collectBooksRecursively(bookSet);
        }
    }

    void collectAllNames(set<string>& names) const {
        if (this->genre != "Library") {
            names.insert(this->genre);
        }
        for (const GenreNode* sub : subGenres) {
            sub->collectAllNames(names);
        }
    }

     void findAllGenreNodes(const string& targetGenre, vector<GenreNode*>& results) const {
        if (this->genre == targetGenre) {
            results.push_back(const_cast<GenreNode*>(this));
        }
        for (GenreNode* sub : subGenres) {
            sub->findAllGenreNodes(targetGenre, results);
        }
    }
};

class Book {
public:
    string title, isbn, author;
    vector<string> genres;
    bool isAvailable;

    Book(string t, string i, string a, vector<string> g)
        : title(t), isbn(i), author(a), genres(g), isAvailable(true) {}
};

class User {
private:
    list<Book*> currentlyBorrowed;
    set<Book*> returnedHistory;

public:
    User() {}

    void borrow(Book* book) {
        currentlyBorrowed.push_back(book);
        returnedHistory.insert(book);
        book->isAvailable = false;
    }

    bool returnBook(Book* book) {
        auto it = find(currentlyBorrowed.begin(), currentlyBorrowed.end(), book);
        if (it != currentlyBorrowed.end()) {
            currentlyBorrowed.erase(it);
            book->isAvailable = true;
            return true;
        }
        return false;
    }
    
    void undoBorrow(Book* book){
        returnedHistory.erase(book);
        auto it = find(currentlyBorrowed.begin(), currentlyBorrowed.end(), book);
        if (it != currentlyBorrowed.end()) {
           currentlyBorrowed.erase(it);
        }
    }
    
    const list<Book*>& getCurrentlyBorrowed() const { return currentlyBorrowed; }
    const set<Book*>& getBorrowingHistory() const { return returnedHistory; }
};

class LibraryManager {
private:
    map<string, Book*> booksByIsbn;
    map<string, Book*> booksByTitle;
    BookGraph bookGraph;
    User* sessionUser;
    GenreNode* rootGenre;
    stack<Action> actionHistory;

    vector<string> split(const string& s, char delimiter) {
        vector<string> tokens;
        string token;
        istringstream tokenStream(s);
        while (getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }

public:
    LibraryManager() {
        sessionUser = new User();
        rootGenre = new GenreNode("Library");
    }

    ~LibraryManager() {
        for (auto const& pair : booksByIsbn) delete pair.second;
        delete sessionUser;
        delete rootGenre;
    }

    void loadBooks(const string& filename) {
        ifstream file(filename);
        string line;
        while (getline(file, line)) {
            stringstream ss(line);
            string title, isbn, author, genrePath;
            getline(ss, title, ',');
            getline(ss, isbn, ',');
            getline(ss, author, ',');
            getline(ss, genrePath, ',');
            
            vector<string> genres = split(genrePath, '>');
            Book* newBook = new Book(title, isbn, author, genres);
            booksByIsbn[isbn] = newBook;
            booksByTitle[title] = newBook;
            bookGraph.addBook(newBook);

            GenreNode* currentNode = rootGenre;
            for (const string& genreName : genres) {
                GenreNode* nextNode = currentNode->findSubGenre(genreName);
                if (!nextNode) {
                    nextNode = new GenreNode(genreName);
                    currentNode->subGenres.push_back(nextNode);
                }
                currentNode = nextNode;
            }
            currentNode->addBook(newBook);
        }
        buildGraphByAuthor();
    }
    
    void buildGraphByAuthor() {
        map<string, vector<Book*>> booksByAuthor;
        for(auto const& pair : booksByIsbn) {
            booksByAuthor[pair.second->author].push_back(pair.second);
        }
        for(auto const& pair : booksByAuthor) {
            const auto& books = pair.second;
            for(size_t i = 0; i < books.size(); ++i) {
                for(size_t j = i + 1; j < books.size(); ++j) {
                    bookGraph.addEdge(books[i], books[j], 1);
                }
            }
        }
    }
    
    User* getSessionUser() const { return sessionUser; }
    Book* findBookByIsbn(const string& isbn) { return booksByIsbn.count(isbn) ? booksByIsbn[isbn] : nullptr; }
    Book* findBookByTitle(const string& title) { return booksByTitle.count(title) ? booksByTitle[title] : nullptr; }
    const map<string, Book*>& getAllBooks() const { return booksByIsbn; }
    const GenreNode* getGenreRoot() const { return rootGenre; }

    vector<string> getUniqueGenreNames() const {
        set<string> names;
        if (rootGenre) {
            rootGenre->collectAllNames(names);
        }
        return vector<string>(names.begin(), names.end());
    }

    bool borrowBook(Book* book) {
        if (book && book->isAvailable) {
            for (Book* borrowedBook : sessionUser->getCurrentlyBorrowed()) {
                bookGraph.addEdge(borrowedBook, book, 2);
            }
            sessionUser->borrow(book);
            actionHistory.push({Action::BORROW, book});
            return true;
        }
        return false;
    }

    bool returnBook(Book* book) {
        if (book && !book->isAvailable) {
            if(sessionUser->returnBook(book)){
                actionHistory.push({Action::RETURN, book});
                return true;
            }
        }
        return false;
    }

    bool undoLastAction() {
        if (actionHistory.empty()) return false;
        
        Action lastAction = actionHistory.top();
        actionHistory.pop();

        if (lastAction.type == Action::BORROW) {
            lastAction.book->isAvailable = true;
            sessionUser->undoBorrow(lastAction.book);
        } else {
            lastAction.book->isAvailable = false;
            sessionUser->borrow(lastAction.book);
        }
        return true;
    }

    vector<Book*> recommendBooks() {
        vector<Book*> recommendations;
        map<Book*, int> recommendationScores;
        const auto& userHistory = sessionUser->getBorrowingHistory();

        if (!userHistory.empty()) {
            for (Book* book : userHistory) {
                BookNode* node = bookGraph.getNode(book);
                if (node) {
                    for (auto const& pair : node->adjacentNodes) {
                        Book* recommendedBook = pair.first->book;
                        if (userHistory.find(recommendedBook) == userHistory.end() && recommendedBook->isAvailable) {
                            recommendationScores[recommendedBook] += pair.second;
                        }
                    }
                }
            }
            
            vector<pair<Book*, int>> sortedScores(recommendationScores.begin(), recommendationScores.end());
            sort(sortedScores.begin(), sortedScores.end(), [](const auto& a, const auto& b) {
                return a.second > b.second;
            });

            for (int i = 0; i < min((int)sortedScores.size(), 3); ++i) {
                recommendations.push_back(sortedScores[i].first);
            }
        }

        if (recommendations.size() < 3) {
            set<Book*> existingRecs(recommendations.begin(), recommendations.end());
            for (auto const& pair : booksByIsbn) {
                Book* book = pair.second;
                if (recommendations.size() >= 3) break;

                bool alreadyInHistory = userHistory.count(book);
                bool alreadyInRecs = existingRecs.count(book);

                if (book->isAvailable && !alreadyInHistory && !alreadyInRecs) {
                    recommendations.push_back(book);
                }
            }
        }

        return recommendations;
    }
};

class LibraryUI {
private:
    LibraryManager& manager;

    void showHeader() {
        cout << "\n=======================================\n";
        cout << "       Library Management System     \n";
        cout << "=======================================\n";
    }
    
    Book* getBookInput(const string& action) {
        int opt;
        while (true) {
            cout << action << " by (1) ISBN or (2) Title? ";
            cin >> opt;
            if (cin.fail()) {
                cout << "Invalid input. Please enter a number.\n";
                cin.clear();
                cin.ignore(10000, '\n');
                continue;
            }
            if (opt == 1 || opt == 2) {
                cin.ignore(10000, '\n');
                break;
            } else {
                cout << "Invalid choice. Please select 1 or 2.\n";
            }
        }
        string value;
        cout << "Enter value: ";
        getline(cin, value);
        if (opt == 1) return manager.findBookByIsbn(value);
        return manager.findBookByTitle(value);
    }

    void showAllBooks() {
        const auto& allBooksMap = manager.getAllBooks();
        if (allBooksMap.empty()) {
            cout << "The library is empty.\n";
            return;
        }

        vector<Book*> allBooks;
        for(const auto& pair : allBooksMap) allBooks.push_back(pair.second);
        
        const int pageSize = 10;
        int totalPages = (allBooks.size() + pageSize - 1) / pageSize;
        int currentPage = 1;
        char choice_char;

        do {
            cout << "\n--- All Books (Page " << currentPage << "/" << totalPages << ") ---\n";
            int start_index = (currentPage - 1) * pageSize;
            int end_index = min((int)allBooks.size(), start_index + pageSize);

            for (int i = start_index; i < end_index; ++i) {
                Book* b = allBooks[i];
                cout << i + 1 << ". " << b->title << " by " << b->author
                     << " (ISBN: " << b->isbn << ") ["
                     << (b->isAvailable ? "Available" : "Borrowed") << "]\n";
            }
            cout << "\n(N)ext, (P)revious, (X)it: ";
            cin >> choice_char;
            cin.ignore(10000, '\n');
            
            choice_char = toupper(choice_char);
            if (choice_char == 'N') {
                if(currentPage < totalPages) currentPage++;
                else cout << "Already on the last page.\n";
            } else if (choice_char == 'P') {
                if(currentPage > 1) currentPage--;
                else cout << "Already on the first page.\n";
            }
        } while (toupper(choice_char) != 'X');
    }

    void searchBook() {
        cout << "\n--- Search Book ---\n";
        Book* book = getBookInput("Search");
        if(book) {
            cout << "Found: " << book->title << " by " << book->author << " [" << (book->isAvailable ? "Available" : "Borrowed") << "]\n";
        } else {
            cout << "Book not found.\n";
        }
    }

    void showBooksByGenre() {
        cout << "\n--- Browse by Genre ---\n";
        vector<string> genres = manager.getUniqueGenreNames();

        if (genres.empty()) {
            cout << "No genres available in the library.\n";
            return;
        }

        cout << "Available genres:\n";
        for (const string& genre : genres) {
            cout << "- " << genre << "\n";
        }
        
        cout << "\nEnter genre name to view books: ";
        string genreName;
        getline(cin, genreName);

        vector<GenreNode*> results;
        manager.getGenreRoot()->findAllGenreNodes(genreName, results);

        if(results.empty()){
            cout << "Genre not found or no books available for this genre.\n";
            return;
        }
        
        set<Book*> uniqueBooks;
        for(GenreNode* node : results){
            node->collectBooksRecursively(uniqueBooks);
        }

        cout << "\n--- Books in Genre: " << genreName << " ---\n";
        for(Book* b : uniqueBooks){
             cout << "- " << b->title << " by " << b->author << " [" << (b->isAvailable ? "Available" : "Borrowed") << "]\n";
        }
    }

    void borrowBook() {
        cout << "\n--- Borrow Book ---\n";
        Book* book = getBookInput("Borrow");
        if(manager.borrowBook(book)) {
            cout << "Book '" << book->title << "' borrowed successfully.\n";
        } else {
            cout << "Failed to borrow book. It might be unavailable or invalid.\n";
        }
    }

    void returnBook() {
        cout << "\n--- Return Book ---\n";
        const auto& borrowed = manager.getSessionUser()->getCurrentlyBorrowed();
        if (borrowed.empty()){
            cout << "You have no books to return.\n";
            return;
        }
        cout << "Your currently borrowed books:\n";
        for(Book* b : borrowed) {
            cout << "- " << b->title << " (ISBN: " << b->isbn << ")\n";
        }
        
        Book* book = getBookInput("Return");
        if(manager.returnBook(book)) {
            cout << "Book '" << book->title << "' returned successfully.\n";
        } else {
            cout << "Failed to return book. Invalid or not borrowed by you.\n";
        }
    }

    void undoAction() {
        if(manager.undoLastAction()){
            cout << "Last action has been successfully undone.\n";
        } else {
            cout << "No action to undo.\n";
        }
    }

    void showRecommendations() {
        cout << "\n--- Recommended For You ---\n";
        vector<Book*> recs = manager.recommendBooks();
        if (recs.empty()) {
            cout << "No available books to recommend at the moment.\n";
        } else {
            for(Book* b : recs) {
                cout << "- " << b->title << " by " << b->author << " (ISBN: " << b->isbn << ")\n";
            }
        }
    }
    
public:
    LibraryUI(LibraryManager& libManager) : manager(libManager) {}

    void run() {
        while (true) {
            showHeader();
            cout << "\n1. Show All Books\n";
            cout << "2. Search Book\n";
            cout << "3. Browse by Genre\n";
            cout << "4. Borrow a Book\n";
            cout << "5. Return a Book\n";
            cout << "6. Undo Last Action\n";
            cout << "7. Show Recommendations\n";
            cout << "8. Exit\n";
            cout << "Choice: ";

            int choice;
            cin >> choice;
            if (cin.fail()) {
                cin.clear();
                cin.ignore(10000, '\n');
                choice = 0;
            }
            cin.ignore(10000, '\n');

            switch (choice) {
                case 1: showAllBooks(); break;
                case 2: searchBook(); break;
                case 3: showBooksByGenre(); break;
                case 4: borrowBook(); break;
                case 5: returnBook(); break;
                case 6: undoAction(); break;
                case 7: showRecommendations(); break;
                case 8: cout << "Exiting...\n"; return;
                default: cout << "Invalid choice.\n";
            }
        }
    }
};

int main() {
    LibraryManager manager;
    manager.loadBooks("books.txt");
    LibraryUI ui(manager);
    ui.run();
    return 0;
}
