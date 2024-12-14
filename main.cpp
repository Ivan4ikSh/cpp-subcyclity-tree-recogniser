#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct GraphProperties {
    bool is_tree;
    bool is_numbered_tree;
    bool is_acyclic;
    bool is_subcyclic;
};

struct CycleData {
    int count;
    std::string path;
};

class Graph {
public:
    Graph(const std::string& filename, std::ostream& output) : output_(output) {
        LoadGraphFromFile(filename);
        SetProperties();
    }

    void SetProperties() {
        properties_.is_acyclic = IsAcyclic();
        properties_.is_subcyclic = IsSubcyclic();
        properties_.is_numbered_tree = IsNumberedTree();
        properties_.is_tree = IsTree();
    }

    void AddEdge(const std::string& v, const std::string& w) {
        adj_[v].push_back(w);
        adj_[w].push_back(v);
    }

    GraphProperties GetProperties() const { return properties_; }

private:
    GraphProperties properties_;
    std::unordered_map<std::string, std::vector<std::string>> adj_;
    std::ostream& output_;

    void DFS(const std::string& v, const std::string& start, std::set<std::string>& path, std::unordered_set<std::string>& visited, std::unordered_set<std::string>& unique_cycles) {
        std::stack<std::pair<std::string, std::vector<std::string>>> stack;

        stack.push({ start, {} });
        while (!stack.empty()) {
            auto [v, current_path] = stack.top();
            stack.pop();

            if (visited.find(v) == visited.end()) {
                visited.insert(v);
                current_path.push_back(v);
                path.insert(v);

                for (const std::string& neighbor : adj_[v]) {
                    if (neighbor == start && current_path.size() > 2) {
                        std::vector<std::string> cycle(current_path);
                        std::sort(cycle.begin(), cycle.end());
                        unique_cycles.insert(Join(cycle));
                    }
                    else if (visited.find(neighbor) == visited.end()) {
                        stack.push({ neighbor, current_path });
                    }
                }

                path.erase(v);
            }
        }

        for (const std::string& node : path) {
            visited.erase(node);
        }
    }

    CycleData SimpleCycleCount() {
        std::unordered_set<std::string> unique_cycles;
        for (const auto& pair : adj_) {
            const std::string& vertex = pair.first;
            std::set<std::string> path;
            std::unordered_set<std::string> visited;
            DFS(vertex, vertex, path, visited, unique_cycles);
        }
        return { static_cast<int>(unique_cycles.size()), !unique_cycles.empty() ? *unique_cycles.begin() : "" };
    }

    std::string Join(const std::vector<std::string>& cycle) {
        std::string result;
        for (const auto& vertex : cycle) {
            result += vertex + "-";
        }
        result += *cycle.begin();
        return result;
    }

    void LoadGraphFromFile(const std::string& file_name) {
        std::ifstream in_file("input/" + file_name);
        if (!in_file.is_open()) throw std::runtime_error("Ошибка открытия файла: " + file_name);

        std::string line;
        while (getline(in_file, line)) {
            std::stringstream ss(line);
            std::string u, v;

            if (ss >> u) {
                if (ss >> v) AddEdge(u, v);
                else if (adj_.find(u) == adj_.end()) adj_[u] = {};
            }
        }
    }

    bool IsTree() const {
        return properties_.is_acyclic && properties_.is_subcyclic;
    }

    bool IsNumberedTree() const {
        int vertex_count = GetVertexCount();
        int edge_count = CountEdges();

        if (properties_.is_acyclic)
            if (properties_.is_subcyclic) return true;
            else return false;
        else if(properties_.is_subcyclic) return false;
        else return (edge_count == vertex_count - 1);
    }

    int CountEdges() const {
        int edge_count = 0;
        for (const auto& neighbors : adj_) {
            edge_count += neighbors.second.size();
        }
        return edge_count / 2;
    }

    bool IsAcyclic() {
        CycleData cycle_data = SimpleCycleCount();

        if (cycle_data.count > 0) {
            output_ << "Нарушение ацикличности. Найденный цикл: ";
            output_ << cycle_data.path << "\n";
            return false;
        }

        return true;
    }

    bool IsSubcyclicException() {
        std::unordered_set<std::string> triangle_vertices;
        std::unordered_set<std::string> edge_vertices;

        for (const auto& pair : adj_) {
            const std::string& v = pair.first;
            const auto& neighbors = pair.second;

            if (neighbors.size() >= 2) {
                for (size_t i = 0; i < neighbors.size(); ++i) {
                    for (size_t j = i + 1; j < neighbors.size(); ++j) {
                        if (AreConnected(neighbors[i], neighbors[j])) {
                            triangle_vertices.insert(v);
                            triangle_vertices.insert(neighbors[i]);
                            triangle_vertices.insert(neighbors[j]);
                        }
                    }
                }
            }

            if (neighbors.size() == 1 || neighbors.size() == 0) {
                edge_vertices.insert(v);
                if (neighbors.size() != 0) edge_vertices.insert(neighbors[0]);
            }
        }

        bool is_triangle_with_edge = triangle_vertices.size() >= 3 && edge_vertices.size() >= 2;
        bool is_triangle_with_isolated_vertex = triangle_vertices.size() >= 3 && edge_vertices.size() == 1;

        return !(is_triangle_with_edge || is_triangle_with_isolated_vertex);
    }

    bool IsSubcyclic() {
        if (!IsSubcyclicException()) {
            output_ << "Нарушена субцикличность. Граф является исключением\n";
            return false;
        }

        for (const auto& pair1 : adj_) {
            for (const auto& pair2 : adj_) {
                const auto& v = pair1.first;
                const auto& w = pair2.first;

                if (v != w && !AreConnected(v, w)) {
                    AddEdge(v, w);
                    if (SimpleCycleCount().count != 1) {
                        RemoveEdge(v, w);
                        output_ << "Нарушена субцикличность в вершинах " << v << "-" << w << "\n";
                        return false;
                    }
                    RemoveEdge(v, w);
                }
            }
        }
        return true;
    }

    bool AreConnected(const std::string& v, const std::string& w) const {
        return std::find(adj_.at(v).begin(), adj_.at(v).end(), w) != adj_.at(v).end();
    }

    void RemoveEdge(const std::string& v, const std::string& w) {
        adj_[v].erase(std::remove(adj_[v].begin(), adj_[v].end(), w), adj_[v].end());
        adj_[w].erase(std::remove(adj_[w].begin(), adj_[w].end(), v), adj_[w].end());
    }

    int GetVertexCount() const { return adj_.size(); }
};

void CheckGraph(const std::string& file_name) {
    try {
        std::ofstream result_file("output/" + file_name);

        if (!result_file.is_open()) throw std::runtime_error("Ошибка открытия файла для записи результатов: " + file_name);

        Graph g(file_name, result_file);
        GraphProperties props = g.GetProperties();

        if (props.is_numbered_tree) result_file << "Граф является древочисленным.\n";
        else result_file << "Граф не является древочисленным.\n";

        if (props.is_tree) result_file << "Граф является деревом.\n";
        else result_file << "Граф не является деревом.\n";

        result_file.close();
        std::cout << "Успех!\nРезульаты работы программы были выведены в output/" + file_name + "\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка при проверке графа из файла '" << file_name << "': " << e.what() << "\n";
    }
}

void LOG(Graph& g, std::ofstream& log_file) {
    double total_time = 0.0;
    for (int i = 0; i < 10; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        g.SetProperties();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        log_file << duration << "мс\n";
        total_time += duration;
    }

    log_file << "Среднее время: " << total_time / 10 << "мс\n";
}

void TEST() {
    CheckGraph("is-tree.txt");

    CheckGraph("ac-err.txt");
    CheckGraph("sub-err.txt");

    CheckGraph("ac-sub-err.txt");
    CheckGraph("ac-sub-exp1-err.txt");
    CheckGraph("ac-sub-exp2-err.txt");
}

int main() {
    setlocale(LC_ALL, "rus");

    std::cout << "Выберете режим работы:\n";
    std::cout << "1. Проверить граф на дерево\n";
    std::cout << "2. Получить данные о скорости выполнения\n";
    int choice;
    std::cin >> choice;
    system("cls");
    switch (choice)
    {
    case 1: {
        std::cout << "Введите имя файла для проверки\n";
        std::cout << "!Важно, что бы файл хранился в директроии input/\n";
        std::string file_name;
        std::cin >> file_name;
        system("cls");
        CheckGraph(file_name);
    }
        break;
    case 2: {
        std::ofstream res_file("log/result.txt");
        std::ofstream log("log/log.txt");

        Graph g1("test1.txt", res_file);
        LOG(g1, log);
        Graph g2("test2.txt", res_file);
        LOG(g2, log);
        Graph g3("test3.txt", res_file);
        LOG(g3, log);
        log.close();
        res_file.close();

        system("cls");
        std::cout << "Успех!\nРезульаты работы программы были выведены в log/log.txt\n";
    }
        break;
    default:
        break;
    }

    return 0;
}