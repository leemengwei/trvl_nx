#include <vector>
#include <unordered_map>
#include <queue>
#include <cstring>

struct HuffmanNode {
    uint32_t freq;
    char c;
    bool is_leaf;
    HuffmanNode* left;
    HuffmanNode* right;

    HuffmanNode(uint32_t f, char ch) 
        : freq(f), c(ch), is_leaf(true), left(nullptr), right(nullptr) {}

    HuffmanNode(HuffmanNode* l, HuffmanNode* r)
        : freq(l->freq + r->freq), is_leaf(false), left(l), right(r), c(0) {}
};

struct Compare {
    bool operator()(HuffmanNode* a, HuffmanNode* b) {
        return a->freq > b->freq;
    }
};

void GenerateCodes(HuffmanNode* node, uint32_t code, int bits, 
                   std::unordered_map<char, std::pair<uint32_t, int>>& code_map) {
    if (!node) return;
    if (node->is_leaf) {
        code_map[node->c] = {code, bits};
        return;
    }
    GenerateCodes(node->left, (code << 1), bits + 1, code_map);
    GenerateCodes(node->right, (code << 1) | 1, bits + 1, code_map);
}

void DeleteTree(HuffmanNode* node) {
    if (!node) return;
    DeleteTree(node->left);
    DeleteTree(node->right);
    delete node;
}

std::vector<char> HuffmanEncode(const std::vector<char>& vle_data) {
    if (vle_data.empty()) return {};

    // 统计频率
    std::unordered_map<char, uint32_t> freq_map;
    for (char c : vle_data) freq_map[c]++;

    // 构建优先队列
    std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, Compare> pq;
    for (const auto& pair : freq_map) 
        pq.push(new HuffmanNode(pair.second, pair.first));

    // 构建哈夫曼树
    while (pq.size() > 1) {
        auto left = pq.top(); pq.pop();
        auto right = pq.top(); pq.pop();
        pq.push(new HuffmanNode(left, right));
    }

    HuffmanNode* root = pq.top();

    // 生成编码表
    std::unordered_map<char, std::pair<uint32_t, int>> code_map;
    GenerateCodes(root, 0, 0, code_map);
    DeleteTree(root);

    // 构建输出数据
    std::vector<char> encoded_data;

    // 写入头信息
    uint16_t m = freq_map.size();
    encoded_data.insert(encoded_data.end(), 
        reinterpret_cast<char*>(&m), reinterpret_cast<char*>(&m) + sizeof(m));

    for (const auto& pair : freq_map) {
        encoded_data.push_back(pair.first);
        uint32_t freq = pair.second;
        encoded_data.insert(encoded_data.end(),
            reinterpret_cast<const char*>(&freq), 
            reinterpret_cast<const char*>(&freq) + sizeof(freq));
    }

    uint32_t data_size = vle_data.size();
    encoded_data.insert(encoded_data.end(),
        reinterpret_cast<char*>(&data_size), 
        reinterpret_cast<char*>(&data_size) + sizeof(data_size));

    // 编码数据
    uint8_t buffer = 0;
    int bit_count = 0;
    for (char c : vle_data) {
        auto& code_info = code_map[c];
        uint32_t code = code_info.first;
        int bits = code_info.second;

        for (int i = bits - 1; i >= 0; --i) {
            buffer = (buffer << 1) | ((code >> i) & 1);
            if (++bit_count == 8) {
                encoded_data.push_back(buffer);
                buffer = bit_count = 0;
            }
        }
    }

    if (bit_count > 0) {
        buffer <<= (8 - bit_count);
        encoded_data.push_back(buffer);
    }

    return encoded_data;
}

std::vector<char> HuffmanDecode(const std::vector<char>& encoded_data) {
    if (encoded_data.size() < sizeof(uint16_t) + sizeof(uint32_t))
        return {};

    size_t pos = 0;
    uint16_t m;
    memcpy(&m, &encoded_data[pos], sizeof(m));
    pos += sizeof(m);

    std::unordered_map<char, uint32_t> freq_map;
    for (int i = 0; i < m; ++i) {
        if (pos + 1 + sizeof(uint32_t) > encoded_data.size())
            return {};
        char c = encoded_data[pos++];
        uint32_t freq;
        memcpy(&freq, &encoded_data[pos], sizeof(freq));
        pos += sizeof(freq);
        freq_map[c] = freq;
    }

    uint32_t data_size;
    memcpy(&data_size, &encoded_data[pos], sizeof(data_size));
    pos += sizeof(data_size);

    // 重建哈夫曼树
    std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, Compare> pq;
    for (const auto& pair : freq_map)
        pq.push(new HuffmanNode(pair.second, pair.first));

    while (pq.size() > 1) {
        auto left = pq.top(); pq.pop();
        auto right = pq.top(); pq.pop();
        pq.push(new HuffmanNode(left, right));
    }

    HuffmanNode* root = pq.top();
    std::vector<char> decoded_data;
    decoded_data.reserve(data_size);

    size_t byte_idx = pos;
    int bit_pos = 0;
    uint8_t current_byte = byte_idx < encoded_data.size() ? 
        encoded_data[byte_idx] : 0;

    HuffmanNode* current = root;
    while (decoded_data.size() < data_size) {
        if (byte_idx >= encoded_data.size()) break;

        uint8_t bit = (current_byte >> (7 - bit_pos)) & 1;
        if (++bit_pos == 8) {
            bit_pos = 0;
            current_byte = (++byte_idx < encoded_data.size()) ? 
                encoded_data[byte_idx] : 0;
        }

        current = bit ? current->right : current->left;

        if (current->is_leaf) {
            decoded_data.push_back(current->c);
            current = root;
        }
    }

    DeleteTree(root);
    return decoded_data;
}