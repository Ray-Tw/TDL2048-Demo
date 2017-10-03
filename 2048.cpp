/**
 * TDL2048-Demo
 * Temporal Difference Learning Demo for Game 2x2 2048 (Demo)
 * Learning TD(0) by a 2x2 2048 after-state agent.
 *
 * use 'g++ -std=c++0x -O3 -g -o 2048 2048.cpp' to compile the source
 * https://github.com/moporgic/TDL2048-Demo
 *
 * Hung Guei
 * Computer Games and Intelligence (CGI) Lab, NCTU, Taiwan
 * http://www.aigames.nctu.edu.tw
 */
#include <array>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <functional>
#include <numeric>

class board {
public:
	board() : tile({}) {}
	board(const board& b) : tile(b.tile) {}
	board(int v) : board() { operator =(v); }
	std::array<int, 2>& operator[](int i) { return tile[i]; }
	const std::array<int, 2>& operator[](int i) const { return tile[i]; }
	operator int() const {
		return tile[0][0] * 4096 + tile[0][1] * 256 + tile[1][0] * 16 + tile[1][1]; }
	board& operator=(int v) {
		tile[0][0] = (v >> 12) & 15;
		tile[0][1] = (v >> 8) & 15;
		tile[1][0] = (v >> 4) & 15;
		tile[1][1] = v & 15;
		return *this;
	}

	int move(const int& opcode) {
		switch (opcode) {
		case 0: return up();
		case 1: return right();
		case 2: return down();
		case 3: return left();
		default: return -1;
		}
	}

	int left() {
		int now = *this;
		int score = 0;
		for (auto& row : tile) {
			if (row[0] == 0) {
				row[0] = row[1];
				row[1] = 0;
			} else if (row[0] == row[1]) {
				row[0]++;
				row[1] = 0;
				score += (1 << row[0]) & -2u;
			}
		}
		return int(*this) != now ? score : -1;
	}
	int right() {
		mirror();
		int score = left();
		mirror();
		return score;
	}
	int up() {
		rotate(1);
		int score = right();
		rotate(-1);
		return score;
	}
	int down() {
		rotate(1);
		int score = left();
		rotate(-1);
		return score;
	}

	void transpose() {
		std::swap(tile[0][1], tile[1][0]);
	}

	void mirror() {
		std::swap(tile[0][0], tile[0][1]);
		std::swap(tile[1][0], tile[1][1]);
	}

	void flip() {
		std::swap(tile[0][0], tile[1][0]);
		std::swap(tile[0][1], tile[1][1]);
	}

	void rotate(const int& r = 1) {
		switch (((r % 4) + 4) % 4) {
		default:
		case 0: break;
		case 1: transpose(); mirror(); break;
		case 2: mirror(); flip(); break;
		case 3: transpose(); flip(); break;
		}
	}

	void isomorphic(const int& i) {
		int iso = ((i % 8) + 8) % 8;
		if (iso > 4) mirror();
		rotate(iso);
	}

	void next() {
		int space[4], num = 0;
		if (tile[0][0] == 0) space[num++] = 0;
		if (tile[0][1] == 0) space[num++] = 1;
		if (tile[1][0] == 0) space[num++] = 2;
		if (tile[1][1] == 0) space[num++] = 3;
		if (num == 0) return;
		int pos = space[std::rand() % num];
		int pop = std::rand() % 10 ? 1 : 2;
		tile[pos / 2][pos % 2] = pop;
	}

	std::string name() const {
		std::stringstream ss;
		char buff[8];
		std::snprintf(buff, sizeof(buff), "%04x", int(*this));
		ss << buff;
		return ss.str();
	}

    friend std::ostream& operator <<(std::ostream& out, const board& b) {
		char buff[32];
		out << "+------+" << std::endl;
		std::snprintf(buff, sizeof(buff), "|%3u%3u|", (1 << b[0][0]) & -2u, (1 << b[0][1]) & -2u);
		out << buff << std::endl;
		std::snprintf(buff, sizeof(buff), "|%3u%3u|", (1 << b[1][0]) & -2u, (1 << b[1][1]) & -2u);
		out << buff << std::endl;
		out << "+------+" << std::endl;
		return out;
	}

private:
	std::array<std::array<int, 2>, 2> tile;
};



int main(int argc, const char* argv[]) {
	std::srand(std::time(nullptr));

	float weight[6 * 6 * 6 * 6] = { 0 };
	auto V = [&](const board& b) -> float& {
		return weight[ (b[0][0] * 6 * 6 * 6) + (b[0][1] * 6 * 6) + (b[1][0] * 6) + (b[1][1]) ];
	};

	float alpha = 0.01;
	int decimal = 4;
	int isomorphic = 8;

	bool forward = true;
	bool backward = false;

	auto norm = [=](const float& v) {
		double base = std::pow(10, decimal);
		return std::round(v * base) / base;
	};

	std::vector<board> history; history.reserve(100);
	std::vector<int> actions; actions.reserve(50);

	auto make_display_buff = []() -> std::array<std::string, 4> { return { "+", "|", "|", "+" }; };
	auto display_buff = [](std::array<std::string, 4>& buff) {
		for (std::string& line : buff) std::cout << line << std::endl;
	};
	auto append_board_at = [&](std::array<std::string, 4>& buff, int i) {
		board b = history[i];
		std::stringstream ss;
		ss << b;
		std::string line;
		for (int i = 0; i < 4 && std::getline(ss, line); i++)
			buff[i] += line.substr(1);
		if (i % 2) {
			int off = 2;
			std::string n_ = b.name();
			buff[3][buff[3].size() - (off+5)] = '[';
			buff[3][buff[3].size() - (off)] = ']';
			std::copy_n(n_.begin(), n_.size(), buff[3].begin() + (buff[3].size() - (off+4)));

			b = history[i - 1];
			int r = b.move(actions[i / 2]);
			std::string r_ = std::to_string(r);
			buff[0][buff[0].size() - r_.size() - (off+2)] = '(';
			buff[0][buff[0].size() - r_.size() - (off+1)] = '+';
			buff[0][buff[0].size() - (off)] = ')';
			std::copy_n(r_.begin(), r_.size(), buff[0].begin() + (buff[0].size() - r_.size() - (off)));
		}
	};

	auto append_action_at = [&](std::array<std::string, 4>& buff, board b, int x) {
		board a[4] = { b, b, b, b };
		int r[4] = { -1 };
		for (int op = 0; op < 4; op++) {
			r[op] = a[op].move(op);
		}
		std::string opname[] = { "^", ">", "v", "<" };
		for (int i = 0; i < 4; i++) {
			std::stringstream ss;
			ss << buff[i] << " " << opname[i] << ": ";
			if (r[i] != -1) {
				ss << r[i] << " + " << norm(V(a[i]));
				if (i == x) ss << " *";
			} else {
				ss << "n/a";
			}
			buff[i] = ss.str();
		}
	};

	auto train_isomorphic = [&](const board& b, float upd) {
		std::vector<int> trained;
		trained.reserve(8);
		for (int i = 0; i < isomorphic; i++) {
			board iso = b;
			iso.isomorphic(i);
			if (std::find(trained.begin(), trained.end(), iso) == trained.end()) {
				trained.push_back(iso);
				V(iso) += upd;
			}
		}
	};

	for (size_t i = 1; true; i++) {
		bool print = true;
		bool bypass = false;

		if (print) std::cout << "episode #" << i << ":" << std::endl;
		board b;
		while (true) {
			b.next();
			history.push_back(b);
			int x = 0;
			board a[4] = { b, b, b, b };
			int r[4] = { -1 };
			float v[4] = { -std::numeric_limits<float>::max() };
			for (int op = 0; op < 4; op++) {
				r[op] = a[op].move(op);
				if (r[op] != -1) {
					v[op] = r[op] + V(a[op]);
					if (v[op] > v[x]) x = op;
				}
			}

			if (print) {
				auto buff = make_display_buff();
				for (size_t i = 0; i < history.size(); i++) {
					append_board_at(buff, i);
				}
				append_action_at(buff, b, x);
				display_buff(buff);
			}

			if (forward && history.size() > 1) {
				float exact = r[x] != -1 ? v[x] : 0;
				float& u = V(history[history.size() - 2]);
				auto upd = alpha * (exact - u);
				int rwd = r[x] != -1 ? r[x] : 0;
//				auto& v = V(history[history.size() - 2]);

				if (print) {
					std::cout << "TD(0): V(" << history[history.size() - 2].name() << ") = ";
					std::cout << norm(u) << " + ";
					std::cout << alpha << " * (" << rwd << " + " << norm(exact - rwd) << " - " << norm(u) << ") = ";
					std::cout << norm(u + upd) << std::endl;
				}

				train_isomorphic(history[history.size() - 2], upd);

			} else if (forward) {
				if (print) {
					std::cout << "TD(0): n/a" << std::endl;
				}
			}

			if (r[x] == -1) break;
			b = a[x];
			history.push_back(b);
			actions.push_back(x);
		}

		if (backward) {
			int r = 0;
			float exact = 0;
			history.pop_back();
			while (history.size()) {
				auto& v = V(history.back());
				auto upd = alpha * (exact - v);

				if (print) {
					std::cout << "TD(0): V(" << history.back().name() << ") = ";
					std::cout << norm(v) << " + " << alpha << " * (" << r << " + " << norm(exact - r) << " - " << norm(v) << ") = ";
					std::cout << norm(v + upd) << std::endl;
				}

				train_isomorphic(history.back(), upd);

				history.pop_back();
				r = board(history.back()).move(actions.back());
				exact = v + r;
				history.pop_back();
				actions.pop_back();
			}
		}

		if (!bypass) std::getchar();
		history.clear();
		actions.clear();
	}

	return 0;
}
