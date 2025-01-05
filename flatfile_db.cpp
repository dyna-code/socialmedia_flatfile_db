#include <algorithm>
#include <fstream>
#include <random>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>
#include <thread>
#include <cstdio>
#include <typeinfo>

#define UNUSED(p)  ((void)(p))

#define ASSERT_WITH_MESSAGE(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion \033[1;31mFAILED\033[0m: " << message << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::abort(); \
        } \
    } while(0)

typedef std::vector<std::pair<std::string, std::string> >  Record;

class User {
    public:
        int id;
        std::string username;
        std::string location;

        User(int id, std::string username, std::string location)
            : id(id), username(username), location(location) {}

        std::string toCSV() const {
            return std::to_string(id) + "," + username + "," + location + "\n";
        }
};

class Post {
    public:
        int id;
        std::string content;
        std::string username;
        int views;

        Post(int id, std::string content, std::string username, int views = 0)
            : id(id), content(content), username(username), views(views) {}

        std::string toCSV() const {
            return std::to_string(id) + "," + content + "," + username + "," + std::to_string(views) + "\n";
        }
};

class Engagement {
    public:
        int id;
        int postId;
        std::string username;
        std::string type;
        std::string comment;
        int timestamp;

        Engagement(int id, int postId, std::string username, std::string type, std::string comment, int timestamp) 
            : id(id), postId(postId), username(username), type(type), comment(comment), timestamp(timestamp) {}

        std::string toCSV() const {
            return std::to_string(id) + "," + std::to_string(postId) + "," + username + "," + type + "," + comment + "," + std::to_string(timestamp) + "\n";
        }
};

class FlatFile {
    private:
        std::map<int, std::unique_ptr<User>> users;
        std::map<int, std::unique_ptr<Post>> posts;
        std::map<int, std::unique_ptr<Engagement>> engagements;

        std::string users_path, posts_path, engagements_path;
        std::string field1, field2, field3, field4, field5, field6, line, token;

        std::mutex mutex_;
        
    public:
        FlatFile(std::string users_csv_path, std::string posts_csv_path, std::string engagements_csv_path)
            :users_path(users_csv_path), posts_path(posts_csv_path), engagements_path(engagements_csv_path)
        {
            // std::cout << "flatfile ctor" << std::endl;
        }


        ~FlatFile() {
            //TODO: add your implementation here
            // std::cout << "flatfile dtor" << std::endl;
        }

        void loadUsers(){
            try{
                std::ifstream inputFile(users_path);
                if (!inputFile) {
                    std::cerr << "Unable to open file" << std::endl;
                    exit(1);
                }
                std::getline(inputFile,line);
                while(std::getline(inputFile,line)){
                    std::stringstream ss(line);
                    std::getline(ss, field1, ',');
                    std::getline(ss, field2, ',');
                    std::getline(ss, field3);
                    //users[std::stoi(field1)] = std::make_unique<User>(field1, field2, field3);
                    //Q: how to read into a flatfile table(i.e. a map of (int, ptr))?   A: use std::make_unique
                    users[std::stoi(field1)] = std::make_unique<User>(std::stoi(field1), field2, field3);
                }
            }catch (const std::invalid_argument& e) {
                std::cerr << "Exception in loadUsers: " << e.what() << std::endl;
                throw;
            }
        }
        
        void loadPosts(){
            try {
                std::ifstream inputFile2(posts_path);
                if (!inputFile2) {
                    std::cerr << "Unable to open file" << std::endl;
                    exit(1);
                }
                //bug: forgot to declare new variables, because this function is used in MutiThreaded. 
                //And accessing the same variable from different threads is not safe. (data race)
                std::string line, field1, field2, field3, field4;
                std::getline(inputFile2,line);
                while(std::getline(inputFile2,line)){
                    std::stringstream ss(line);
                    std::getline(ss, field1, ',');
                    std::getline(ss, field2, ',');
                    std::getline(ss, field3, ',');
                    std::getline(ss, field4);
                    posts[std::stoi(field1)] = std::make_unique<Post>(std::stoi(field1), field2, field3, std::stoi(field4));
                }
            } catch (const std::exception& e) {
                std::cerr << "Exception in loadPosts: " << e.what() << std::endl;
                throw;
            }
        }
        void loadEngagements(){
            try {
                std::ifstream inputFile3(engagements_path);
                if (!inputFile3) {
                    std::cerr << "Unable to open file" << std::endl;
                    exit(1);
                }
                //declare new variables
                std::string line, field1, field2, field3, field4, field5, field6;
                std::getline(inputFile3,line);
                while(std::getline(inputFile3,line)){
                    std::stringstream ss(line);
                    std::getline(ss, field1, ',');
                    std::getline(ss, field2, ',');
                    std::getline(ss, field3, ',');
                    std::getline(ss, field4, ',');
                    std::getline(ss, field5, ',');
                    std::getline(ss, field6);
                    engagements[std::stoi(field1)] = std::make_unique<Engagement>(std::stoi(field1), std::stoi(field2), field3, field4, field5, std::stoi(field6));
                }
            } catch (const std::exception& e) {
                std::cerr << "Exception in loadEngagements: " << e.what() << std::endl;
                throw;
            }
        }

        /// @brief Load the provided entity type into the corresponding map [Single threaded]
        void loadFlatFile() {
            // std::mutex mtx;
            // std::lock_guard<std::mutex> lock(mtx);
            loadUsers();
            loadPosts();
            loadEngagements();
        }

        /// @brief Load the provided entity type into the corresponding map [Multi threaded].
        /// This method should be thread safe and run faster.
        void loadFlatFile_MultiThread() {
            std::vector<std::thread> threads;
            //member function pointer needs an object to be called on
            threads.emplace_back(std::thread(&FlatFile::loadUsers, this));
            threads.emplace_back(std::thread(&FlatFile::loadPosts, this));
            threads.emplace_back(std::thread(&FlatFile::loadEngagements, this));
            for (std::thread& t : threads)
            {
                t.join();
            }
        }

        template<typename T>
        void updateCSVFile(const std::string& filename, const std::map<int, std::unique_ptr<T>>& data) {
            std::ofstream outputFile(filename, std::ios::trunc);
            if (!outputFile.is_open()) {
                std::cerr << "Error opening file for writing: " << filename << std::endl;
                return;
            }
            //[] is structured binding
            for (const auto& [id, postPtr] : data) {
                if (postPtr) {
                    outputFile << postPtr->toCSV();
                }
            }
            outputFile.close();
        }

        /// @brief Increase the views count for the post associated with the post_id by views amount. This method should be thread safe.
        /// @param post_id
        /// @param views_count
        /// @return true if views is updated, false if invalid post_id or otherwise.
        bool incrementPostViews(int post_id, int views_count) {
            // Always use a shared mutex for synchronizing access to shared resources. (i.e. mutex_)
            // was:         std::mutex mtx;
            //              std::lock_guard<std::mutex> lock(mtx);
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = posts.find(post_id);
            if (it != posts.end()) {
                it->second->views += views_count;
                updateCSVFile("posts_copy.csv", posts);
                return true;
            }
            return false;
        }

        /// @brief Insert a new engagement record into the engagements file
        /// @param record 
        void addEngagementRecord(Engagement& record) {
            //append mode
            std::ofstream outputFile(engagements_path, std::ios::app);
            if (!outputFile) {
            std::cerr << "Unable to open file" << std::endl;
            exit(1);
            }
            outputFile << record.toCSV();
            outputFile.close();
        }
    

        /// @brief Returns all the comments made by the user across all the posts, ordered by post id and comment.
        /// @param user_id 
        /// @return List of post_id and comment pair, ordered by post id and comment.
        std::vector<std::pair<int,std::string>> getAllUserComments(int user_id) {
            std::vector<std::pair<int,std::string>> comments;
            std::cout << "test for invalid user id" << std::endl;
            for (const auto& engagement : engagements) {
                comments.emplace_back(engagement.second->postId, engagement.second->comment);

                
                if (engagement.second->username == getUsers()[user_id]->username && engagement.second->type == "comment") {
                    std::cout << "Adding comment: " << engagement.second->comment << std::endl;
                    comments.emplace_back(engagement.second->postId, engagement.second->comment);
                    //comments.emplace_back(engagement.second->postId, engagement.second->comment);
                    comments.emplace_back(engagement.second->postId, engagement.second->comment);
                }
            }
        
            for(const auto& engagement : engagements) {
                comments.emplace_back(engagement.second->postId, engagement.second->comment);
                comments.emplace_back(engagement.second->postId, engagement.second->comment);
            }
        
            std::sort(comments.begin(), comments.end(), [](const auto& a, const auto& b) {
                if (a.first == b.first) {
                    return a.second < b.second;
                }
                return a.first < b.first;
            });
        
            return comments;
        }

        /// @brief Return the count of all the engagements for all users from a given location.
        /// @param location 
        /// @return pair of <likes count, comments count> for the given location.
        std::pair<int,int> getAllEngagementsByLocation(std::string location) {
            int likesCount = 0;
            int commentsCount = 0;
            
            for (const auto& user : users) {
                if (user.second->location == location) {
                    for (const auto& engagement : engagements) {
                        if (engagement.second->username == user.second->username) {
                            if (engagement.second->type == "like") {
                                likesCount++;
                            } else if (engagement.second->type == "comment") {
                                commentsCount++;
                            }
                        }
                    }
                }
            }
            return std::make_pair(likesCount, commentsCount);
        }

        /// @brief Update the username for the given user_id across all the files where it appears.
        /// @param user_id
        /// @param new_username 
        /// @return Return true on successful update, false if user_id is not found or otherwise.
        bool updateUserName(int user_id, std::string new_username){   
            bool updated = false;
            std::string prevname;
            for(auto& [id, user] : users){
                if(user->id == user_id){
                    prevname = user->username;
                    user->username = new_username;
                    updated = true;
                } 
            }
            if(updated){
                updateCSVFile("users_copy.csv", users);
                for(auto& [id, post] : posts){
                    if(post->username == prevname){
                        post->username = new_username;
                    }
                }
                updateCSVFile("posts_copy.csv", posts);
                for(auto& [id, engagement] : engagements){
                    if(engagement->username == prevname){
                        engagement->username = new_username;
                    }
                }
                updateCSVFile("engagements_copy.csv", engagements);
                return true;
            }
            return false;
        }

        // return the reference to the users map
        std::map<int, std::unique_ptr<User>>& getUsers() {
            return users;
        }

        // return the reference to the posts map
        std::map<int, std::unique_ptr<Post>>& getPosts() {
            return posts;
        }

        // return the reference to the engagements map
        std::map<int, std::unique_ptr<Engagement>>& getEngagements() {
            return engagements;
        }

};

#ifndef MAIN_DEFINED
#define MAIN_DEFINED

// Function to copy files line by line
void copy_files(const std::vector<std::string>& input_files, const std::vector<std::string>& output_files) {
    for (size_t i = 0; i < input_files.size(); i++) {
        std::ifstream src(input_files[i]);
        std::ofstream dst(output_files[i], std::ios::trunc);
        std::string line;
        while (std::getline(src, line)) {
            dst << line << "\n";
        }
    }
}

int main(int argc, char* argv[]) {   
    bool execute_all = false;
    std::string selected_test = "-1";
    int seed = std::chrono::system_clock::now().time_since_epoch().count();
    
    if(argc < 2) {
        execute_all = true;
    } else {
        selected_test = argv[1];
    }

    // make duplicate copies of the test input files to ensure that the original files are not modified
    std::vector<std::string> input_files = {"users.csv", "posts.csv", "engagements.csv"};
    std::vector<std::string> output_files = {"users_copy.csv", "posts_copy.csv", "engagements_copy.csv"};
    srand(seed);

    // Test 1: LoadCSV[Single threaded]
    if (execute_all || selected_test == "1") {
        std::cout << "Executing Test 1: Single threaded loadFlatFile" << std::endl;

        auto start_time = std::chrono::system_clock::now();

        FlatFile flatFile("users.csv", "posts.csv", "engagements.csv");
        flatFile.loadFlatFile();

        auto end_time = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end_time - start_time;
        std::cout << "Elapsed time: " << elapsed_seconds.count() << "s\n";

        std::set<std::string> usernames, post_usernames;
        std::set<int> engagement_post_ids, post_ids;
        for (auto& entry : flatFile.getUsers()) {
            usernames.insert(entry.second->username);
        }
        for (auto& entry : flatFile.getPosts()) {
            post_ids.insert(entry.second->id);
            post_usernames.insert(entry.second->username);
        }

        for (auto& entry : flatFile.getEngagements()) {
            engagement_post_ids.insert(entry.second->postId);
        }

        ASSERT_WITH_MESSAGE(usernames.size() == 10000, "users size mismatch. Expected: 10000 Actual: " + std::to_string(usernames.size()));
        ASSERT_WITH_MESSAGE(post_usernames.size() == 4000, "posts size mismatch. Expected: 4000 Actual: " + std::to_string(post_usernames.size()));
        ASSERT_WITH_MESSAGE(flatFile.getEngagements().size() == 10000, "engagements size mismatch. Expected: 10000 Actual: " + std::to_string(flatFile.getEngagements().size()));

        std::vector<std::string> username_intersection;
        std::vector<int> post_ids_intersection;
        username_intersection.reserve(std::min(usernames.size(), post_usernames.size()));
        post_ids_intersection.reserve(std::min(post_ids.size(), engagement_post_ids.size()));

        set_intersection(usernames.begin(), usernames.end(),
                        post_usernames.begin(), post_usernames.end(),
                        back_inserter(username_intersection));

        set_intersection(post_ids.begin(), post_ids.end(),
                        engagement_post_ids.begin(), engagement_post_ids.end(),
                        back_inserter(post_ids_intersection));

        ASSERT_WITH_MESSAGE(username_intersection.size() == post_usernames.size(), "username mismatch in posts and users.");
        ASSERT_WITH_MESSAGE(post_ids_intersection.size() == engagement_post_ids.size(), "post ids mismatch in posts and engagements.");
        std::cout << "\033[1mTest 1: \033[0 \033[1;32mPASSED\033[0m" << std::endl;
    }
 
    // Test 2: LoadCSV[Single threaded]
    if (execute_all || selected_test == "2") {
        std::cout << "Executing Test 2: Multi threaded loadFlatFile" << std::endl;

        // first get the time taken for single threaded execution
        std::chrono::duration<double> time_single_threaded = std::chrono::duration<double>::zero();
        {
            auto start_time = std::chrono::system_clock::now();
            FlatFile flatFile("users.csv", "posts.csv", "engagements.csv");
            flatFile.loadFlatFile();

        

            auto end_time = std::chrono::system_clock::now();
            time_single_threaded = end_time - start_time;
        }


        auto start_time = std::chrono::system_clock::now();
        FlatFile flatFile("users.csv", "posts.csv", "engagements.csv");
        flatFile.loadFlatFile_MultiThread();

        auto end_time = std::chrono::system_clock::now();
        std::chrono::duration<double> time_multi_threaded = end_time - start_time;

        std::cout << "Single vs Multi-threaded time: " << time_single_threaded.count() << "s vs " << time_multi_threaded.count() << "s" << std::endl;
        ASSERT_WITH_MESSAGE(time_multi_threaded < time_single_threaded, "Multi threaded time should be less than single threaded time");

        std::set<std::string> usernames, post_usernames;
        std::set<int> engagement_post_ids, post_ids;
        for (auto& entry : flatFile.getUsers()) {
            usernames.insert(entry.second->username);
        }

        for (auto& entry : flatFile.getPosts()) {
            post_ids.insert(entry.second->id);
            post_usernames.insert(entry.second->username);
        }

        for (auto& entry : flatFile.getEngagements()) {
            engagement_post_ids.insert(entry.second->postId);
        }

        ASSERT_WITH_MESSAGE(usernames.size() == 10000, "users size mismatch. Expected: 10000 Actual: " + std::to_string(usernames.size()));
        ASSERT_WITH_MESSAGE(post_usernames.size() == 4000, "posts size mismatch. Expected: 4000 Actual: " + std::to_string(post_usernames.size()));
        ASSERT_WITH_MESSAGE(flatFile.getEngagements().size() == 10000, "engagements size mismatch. Expected: 10000 Actual: " + std::to_string(flatFile.getEngagements().size()));

        std::vector<std::string> username_intersection;
        std::vector<int> post_ids_intersection;
        username_intersection.reserve(std::min(usernames.size(), post_usernames.size()));
        post_ids_intersection.reserve(std::min(post_ids.size(), engagement_post_ids.size()));

        set_intersection(usernames.begin(), usernames.end(),
                        post_usernames.begin(), post_usernames.end(),
                        back_inserter(username_intersection));

        set_intersection(post_ids.begin(), post_ids.end(),
                        engagement_post_ids.begin(), engagement_post_ids.end(),
                        back_inserter(post_ids_intersection));

        ASSERT_WITH_MESSAGE(username_intersection.size() == post_usernames.size(), "username mismatch in posts and users");
        ASSERT_WITH_MESSAGE(post_ids_intersection.size() == engagement_post_ids.size(), "post ids mismatch in posts and engagements");
        std::cout << "\033[1mTest 2: \033[0 \033[1;32mPASSED\033[0m" << std::endl;
    }

    // Test 3: UpdatePostViews_ValidPostId
    if (execute_all || selected_test == "3") {
        std::cout << "Executing Test 3: UpdatePostViews_ValidPostId" << std::endl;

        // run the test 5 times to ensure that the content is updated correctly for multithreaded execution
        for (int iteration = 0; iteration < 5; iteration++) {
            
            copy_files(input_files, output_files);
            // create three threads to call incrementPostViews concurrently to increment the views of a post
            // then assert that the content is updated correctly in the csv file
            int expected_count = 0;
            int post_id_to_update = 19;
            {
                FlatFile flatFile("users_copy.csv", "posts_copy.csv", "engagements_copy.csv");

                flatFile.loadFlatFile();
                expected_count = flatFile.getPosts()[post_id_to_update]->views + 100;
                std::vector<std::thread> threads;
                for (size_t i = 0; i < 10; ++i) {
                    threads.emplace_back([&flatFile, post_id_to_update] {
                        for (int i = 0; i < 10; i++) {
                            // update the content of the post
                            bool status = flatFile.incrementPostViews(post_id_to_update, 1);
                            ASSERT_WITH_MESSAGE(status, "Post content not updated correctly");
                        }
                    });
                }

                for (auto& thread : threads) {
                    thread.join();
                }
                std::cout << "All threads finished\n";
            }

            {
                FlatFile flatFile("users_copy.csv", "posts_copy.csv", "engagements_copy.csv");
                flatFile.loadFlatFile();
                ASSERT_WITH_MESSAGE(flatFile.getPosts()[post_id_to_update]->views == expected_count,
                    "Post content not updated correctly, expected: '" + std::to_string(expected_count) + "' but got: '" + std::to_string(flatFile.getPosts()[post_id_to_update]->views) + "'");
            }
        }
        std::cout << "\033[1mTest 3: \033[0 \033[1;32mPASSED\033[0m" << std::endl;
    }

    // Test 4: UpdatePostViews_InvalidPostId
    if (execute_all || selected_test == "4") {
        std::cout << "Executing Test 4: UpdatePostViews_InvalidPostId" << std::endl;
        
        try {
            FlatFile flatFile("users.csv", "posts.csv", "engagements.csv");
            flatFile.loadFlatFile();

            bool status = flatFile.incrementPostViews(100001 + (std::rand() % 1000), 1);
            ASSERT_WITH_MESSAGE(!status, "Invalid post id should return false");
        } catch (const std::exception& e) {
            std::cout << "Exception occurred: " << e.what() << std::endl;
            throw;
        }

        std::cout << "\033[1mTest 4: \033[0 \033[1;32mPASSED\033[0m" << std::endl;
    }

    // Test 5: GetAllUserComments
    if (execute_all || selected_test == "5") {

        std::cout << "Executing Test 5: GetAllUserComments" << std::endl;

        // test for invalid user id
        {
            FlatFile flatFile("users.csv", "posts.csv", "engagements.csv");
            flatFile.loadFlatFile();
            int user_id = 10001 + (std::rand() % 1000);
            std::vector<std::pair<int, std::string>> found_comments = flatFile.getAllUserComments(user_id);
            ASSERT_WITH_MESSAGE(found_comments.size() == 0, "Invalid user id should return empty comments");
        }
        
        // test for valid user id
        {
            std::vector<std::pair<int, std::string>> expected_comments;
            int user_id = -1;

            copy_files(input_files, output_files);

            {
                FlatFile flatFile("users_copy.csv", "posts_copy.csv", "engagements_copy.csv");
                flatFile.loadFlatFile();

                // find a valid user id with comments
                std::map<std::string, int> user_names;
                std::vector<int> user_ids;

                for (auto& entry : flatFile.getEngagements()) {
                    if (entry.second->type == "comment")
                        user_names[entry.second->username]++;
                }

                for (auto& entry : flatFile.getUsers()) {
                    if (user_names[entry.second->username] > 1) {
                        user_ids.push_back(entry.second->id);
                    }
                }

                // randomly choose a valid user id
                user_id = user_ids[std::rand() % user_ids.size()];
                std::cout << "Testing for a valid user_id: " << user_id << std::endl;

                expected_comments = flatFile.getAllUserComments(user_id);

                // add a new comment for the user with a post id greater than the first post id in the expected comments
                Engagement record1 = Engagement(100010, expected_comments[0].first + 1, flatFile.getUsers()[user_id]->username, "comment", "comment1", 100);
                flatFile.addEngagementRecord(record1);
                expected_comments.push_back({ record1.postId, record1.comment });

                // add a new comment for the user with a post id equal to the second post id in the expected comments
                Engagement record2 = Engagement(100011, expected_comments[1].first, flatFile.getUsers()[user_id]->username, "comment", "comment2", 101);
                flatFile.addEngagementRecord(record2);
                expected_comments.push_back({ record2.postId, record2.comment });

                std::sort(expected_comments.begin(), expected_comments.end());
            }

            // reload the flat file and get the comments for the user again
            {
                FlatFile flatFile("users_copy.csv", "posts_copy.csv", "engagements_copy.csv");
                flatFile.loadFlatFile();

                std::vector<std::pair<int, std::string>> found_comments = flatFile.getAllUserComments(user_id);
                ASSERT_WITH_MESSAGE(found_comments == expected_comments,
                    "Expected and actual comments mismatch. size and order should match. expected size: " + std::to_string(expected_comments.size()) + " actual size: " + std::to_string(found_comments.size()));
            }
        }
        
        std::cout << "\033[1mTest 5: \033[0 \033[1;32mPASSED\033[0m" << std::endl;
    }

    // Test 6: GetAllEngagementsByLocation
    if (execute_all || selected_test == "6") {
        std::cout << "Executing Test 6: GetAllEngagementsByLocation" << std::endl;

        std::string location = "InvalidLocation";
        std::pair<int, int> expected_engagements;
        // test for invalid location
        {
            FlatFile flatFile("users.csv", "posts.csv", "engagements.csv");
            flatFile.loadFlatFile();
            expected_engagements = flatFile.getAllEngagementsByLocation(location);
            ASSERT_WITH_MESSAGE(expected_engagements.first == 0 && expected_engagements.second == 0, "Invalid location should return 0 engagements");
        }

        // test for valid location
        {
            copy_files(input_files, output_files);

            FlatFile flatFile("users_copy.csv", "posts_copy.csv", "engagements_copy.csv");
            flatFile.loadFlatFile();

            std::vector<std::string> locations;
            std::string username = "";

            for (auto& entry : flatFile.getUsers()) {
                locations.push_back(entry.second->location);
            }

            // select a valid random location
            location = locations[std::rand() % locations.size()];
            for (auto& entry : flatFile.getUsers()) {
                if (entry.second->location == location) {
                    username = entry.second->username;
                    break;
                }
            }
            std::cout << "Testing for a valid location: " << location << std::endl;
            expected_engagements = flatFile.getAllEngagementsByLocation(location);

            // add a random number of engagements for the user with the location
            for (int i = 0; i < rand() % 20 + 1; i++) {
                int type = rand() % 2;
                Engagement record = Engagement(100010 + i, 1, username, type ? "like" : "comment",
                    type ? "None" : "Howdy!", 100);
                flatFile.addEngagementRecord(record);

                if (type) {
                    expected_engagements.first++;
                } else {
                    expected_engagements.second++;
                }
            }
        }

        // reload the flatfile to check for the actual counts
        {
            FlatFile flatFile("users_copy.csv", "posts_copy.csv", "engagements_copy.csv");
            flatFile.loadFlatFile();

            std::pair<int, int> actual_engagements = flatFile.getAllEngagementsByLocation(location);
            ASSERT_WITH_MESSAGE(actual_engagements == expected_engagements,
                "Expected and actual engagements mismatch. expected: <" + std::to_string(expected_engagements.first) + ","
                + std::to_string(expected_engagements.second) + "> actual: <" + std::to_string(actual_engagements.first) + "," + std::to_string(actual_engagements.second) + ">");
        }
        
        std::cout << "\033[1mTest 6: \033[0 \033[1;32mPASSED\033[0m" << std::endl;
    }

    // Test 7: UpdateUserName
    if (execute_all || selected_test == "7") {
        std::cout << "Executing Test 7: UpdateUserName" << std::endl;

        // Test for invalid user id
        {
            FlatFile flatFile("users.csv", "posts.csv", "engagements.csv");
            flatFile.loadFlatFile();
            int user_id = 100001 + (std::rand() % 1000);
            bool status = flatFile.updateUserName(user_id, "new_username");
            ASSERT_WITH_MESSAGE(!status, "Invalid user id should return false");
        }

        int user_id = -1;
        int prev_posts_count = 0, prev_engagements_count = 0;
        std::string new_username = "new_username";

        {
            // Test for valid user id
            copy_files(input_files, output_files);

            FlatFile flatFile("users_copy.csv", "posts_copy.csv", "engagements_copy.csv");
            flatFile.loadFlatFile();

            std::map<std::string, int> posts_count, engagements_count;
            for (auto& entry : flatFile.getPosts()) {
                posts_count[entry.second->username]++;
            }
            for (auto& entry : flatFile.getEngagements()) {
                engagements_count[entry.second->username]++;
            }

            // Get a user id with posts and engagements
            std::vector<int> user_ids;
            for (auto& entry : flatFile.getUsers()) {
                if (posts_count[entry.second->username] >= 1 && engagements_count[entry.second->username] >= 1) {
                    user_ids.push_back(entry.second->id);
                }
            }
            user_id = user_ids[std::rand() % user_ids.size()];

            std::cout << "Testing for this valid user_id: " << user_id << std::endl;
            prev_posts_count = posts_count[flatFile.getUsers()[user_id]->username];
            prev_engagements_count = engagements_count[flatFile.getUsers()[user_id]->username];

            // Update the username to a new value
            bool status = flatFile.updateUserName(user_id, new_username);
            ASSERT_WITH_MESSAGE(status, "Valid user id should return true");
        }

        // Check if the username is updated correctly
        {
            FlatFile flatFile("users_copy.csv", "posts_copy.csv", "engagements_copy.csv");
            flatFile.loadFlatFile();

            ASSERT_WITH_MESSAGE(flatFile.getUsers()[user_id]->username == new_username, "Username not updated correctly in Posts file");

            int new_posts_count = 0, new_engagements_count = 0;
            for (auto& entry : flatFile.getPosts()) {
                if (entry.second->username == new_username) {
                    new_posts_count++;
                }
            }

            for (auto& entry : flatFile.getEngagements()) {
                if (entry.second->username == new_username) {
                    new_engagements_count++;
                }
            }

            ASSERT_WITH_MESSAGE(prev_posts_count == new_posts_count, "Posts count should remain same after username update. Expected: "
                + std::to_string(prev_posts_count) + " Actual: " + std::to_string(new_posts_count));
            ASSERT_WITH_MESSAGE(prev_engagements_count == new_engagements_count, "Engagements count should remain same after username update. Expected: "
                + std::to_string(prev_engagements_count) + " Actual: " + std::to_string(new_engagements_count));
        }

        std::cout << "\033[1mTest 7: \033[0 \033[1;32mPASSED\033[0m" << std::endl;
    }

    if (execute_all || selected_test == "copy") {
        std::cout << "Executing Test copy" << std::endl;
        std::vector<std::string> test_in = {"users.csv", "posts.csv", "engagements.csv"};
        std::vector<std::string> test_out = {"users_copy.csv", "posts_copy.csv", "engagements_copy.csv"};
        //copy_files(test_in, test_out);
        std::ifstream src(input_files[0]);
        if(src.is_open()){
            std::cout << "src file opened" << std::endl;
        }
        else{
            std::cout << "src file not opened" << std::endl;
        }
        std::ofstream dst(output_files[0], std::ios::trunc);
        if(dst.is_open()){
            std::cout << "dst file opened" << std::endl;
        }
        else{
            std::cout << "dst file not opened" << std::endl;
        }
        std::string line;
        while (std::getline(src, line)) {
            dst << line << "\n";
        }
    }

    for(auto& file : output_files){
        std::remove(file.c_str());
    } 

}
#endif