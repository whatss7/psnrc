#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

struct pixel {
	unsigned r, g, b;
	pixel() : r(0), g(0), b(0) {}
	pixel(int r, int g, int b) : r(r), g(g), b(b) {}
};

struct picture {
	int width;
	int height;
	int pixel_max;
	std::vector<pixel> pixels;
	picture() : height(0), width(0), pixel_max(255), pixels() {}
	picture(int height, int width, const std::vector<pixel> &pixels,
	        int pixel_max = 255)
	    : height(height), width(width), pixels(pixels), pixel_max(pixel_max) {}
	pixel &operator[](int index) { return pixels[index]; }
	int size() { return width * height; }
};

picture ParsePPM(std::istream &in) {
	std::vector<pixel> result;
	// 0 - magic number / 1 - width / 2 - height / 3 - pixel_max
	// 4 - content (text) / 5 - content (binary)
	int part = 0;
	int width = 0;
	int height = 0;
	int pixel_max = 0;
	bool has_context = false;
	std::string magic;

	pixel pix;
	auto check_context = [&has_context]() {
		if (has_context) {
			has_context = false;
			return true;
		} else return false;
	};
	auto try_next_step = [check_context, &part]() {
		if (check_context()) part++;
	};
	auto append_char = [](int &base, char digit) {
		base = base * 10 + digit - '0';
	};
	int append_pix_color = 0;
	int append_pix_count = 0;
	pixel append_pix_pixel;
	auto append_pix = [&result, &append_pix_count, &append_pix_pixel,
	                   &append_pix_color]() {
		switch (append_pix_count % 3) {
		case 0:
			append_pix_pixel.r = append_pix_color;
			break;
		case 1:
			append_pix_pixel.g = append_pix_color;
			break;
		case 2:
			append_pix_pixel.b = append_pix_color;
			result.push_back(append_pix_pixel);
			append_pix_pixel = pixel();
			break;
		}
		append_pix_count++;
		append_pix_color = 0;
	};
	while (true) {
		int ch = in.get();
		// std::cout << ch << std::endl;
		if (part == 0) {
			if (isspace(ch)) try_next_step();
			else {
				magic += ch;
				has_context = true;
			}
		} else if (part == 1) {
			if (isspace(ch)) try_next_step();
			else if (isdigit(ch)) {
				append_char(width, ch);
				has_context = true;
			} else throw std::invalid_argument("Invalid width.");
		} else if (part == 2) {
			if (isspace(ch)) try_next_step();
			else if (isdigit(ch)) {
				append_char(height, ch);
				has_context = true;
			} else throw std::invalid_argument("Invalid height.");
		} else if (part == 3) {
			if (isspace(ch)) {
				if (check_context()) {
					if (magic == "P3") part = 4;
					else if (magic == "P6") part = 5;
					else
						throw std::invalid_argument(
						    "Invalid magic number, found \"" + magic +
						    "\", excepted \"P3\" or \"P6\"");
				}
			} else if (isdigit(ch)) {
				append_char(pixel_max, ch);
				has_context = true;
			} else throw std::invalid_argument("Invalid maximum color value.");
		} else if (part == 4) {
			if (ch == EOF) {
				if (check_context()) append_pix();
				break;
			} else if (isdigit(ch)) {
				append_char(append_pix_color, ch);
				has_context = true;
			} else if (isspace(ch)) {
				if (check_context()) append_pix();
			} else throw std::invalid_argument("Invalid PPM content.");
		} else if (part == 5) {
			if (ch == EOF) break;
			else {
				append_pix_color = ch;
				append_pix();
			}
		}
	}
	if (append_pix_count % 3 != 0)
		throw std::invalid_argument("Incomplete pixel.");
	if (result.size() != height * width)
		throw std::invalid_argument("Pixel count doesn't match picture size.");
	return picture(height, width, result, pixel_max);
}

int main(int argc, const char **argv) {
	if (argc != 3) {
		std::cerr << "error: " << argv[0] << " takes 2 arguments, but "
		          << argc - 1 << " was given.\n";
		return -1;
	}
	std::ifstream pic1_stream(argv[1], std::ios::binary);
	if (!pic1_stream) {
		std::cerr << "error: failed to open 1st picture \"" << argv[1]
		          << "\".\n";
		return -1;
	}
	picture pic1;
	try {
		pic1 = ParsePPM(pic1_stream);
	} catch (std::invalid_argument ex) {
		std::cerr << "Failed to parse 1st picture \"" << argv[1]
		          << "\": " << ex.what() << "\n";
		return -1;
	}
	pic1_stream.close();
	picture pic2;
	std::ifstream pic2_stream(argv[2], std::ios::binary);
	if (!pic2_stream) {
		std::cerr << "error: failed to open 2nd picture \"" << argv[2]
		          << "\".\n";
		return -1;
	}
	try {
		pic2 = ParsePPM(pic2_stream);
	} catch (std::invalid_argument ex) {
		std::cerr << "error: Failed to parse 2nd picture \"" << argv[2]
		          << "\": " << ex.what() << "\n";
		return -1;
	}
	pic2_stream.close();
	if (pic1.height != pic2.height || pic1.width != pic2.width) {
		std::cerr << "error: Picture sizes don't match. (" << pic1.width << "x"
		          << pic1.height << " vs " << pic2.width << "x" << pic2.height
		          << ")\n";
		return -1;
	}
	double mse = 0;
	auto diff = [](pixel a, pixel b, int pma, int pmb) {
		auto diff_internal = [pma, pmb](int a, int b) {
			double af = double(a) / pma;
			double bf = double(b) / pmb;
			return (af - bf) * (af - bf);
		};
		return (diff_internal(a.r, b.r) + diff_internal(a.g, b.g) +
		        diff_internal(a.b, b.b)) /
		       3;
	};
	for (int i = 0; i < pic1.size(); i++) {
		mse += diff(pic1[i], pic2[i], pic1.pixel_max, pic2.pixel_max);
	}
	mse /= pic1.size();
	std::cout << -10 * log10(mse) << "\n";
	return 0;
}
