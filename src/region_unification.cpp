#include <opencv2/opencv.hpp>
#include <vector>
#include <set>
#include <iostream>
#include <cmath>

using namespace cv;
using namespace std;

struct Region {
    int pixel_count = 0;
    Vec3f mean_color = Vec3f(0, 0, 0);
};

float colorDist(const Vec3f& a, const Vec3f& b)
{
    return norm(a - b);
}

vector<Mat> cached_results;
int merge_level = 0;

void onTrackbar(int, void*)
{
    imshow("Region Unification", cached_results[merge_level]);
}

int main()
{
    cout << "Program start" << endl;

    // read image
    Mat img = imread("images/baboon.jpg", IMREAD_COLOR);

    if (img.empty()) {
        cout << "Image load failed!" << endl;
        return -1;
    }

    int rows = img.rows;
    int cols = img.cols;

    // initial label
    Mat labels(rows, cols, CV_32S);
    vector<Region> regions;
    int next_label = 0;
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {

            bool assigned = false;

            if (y > 0 && img.at<Vec3b>(y, x) == img.at<Vec3b>(y - 1, x)) {
                labels.at<int>(y, x) = labels.at<int>(y - 1, x);
                assigned = true;
                if (x > 0 && img.at<Vec3b>(y, x) == img.at<Vec3b>(y, x - 1)) {
                    labels.at<int>(y, x - 1) = labels.at<int>(y, x);
                }
            }

            if (!assigned && x > 0 && img.at<Vec3b>(y, x) == img.at<Vec3b>(y, x - 1)) {
                labels.at<int>(y, x) = labels.at<int>(y, x - 1);
                assigned = true;
            }

            if (!assigned) {
                labels.at<int>(y, x) = next_label;
                regions.push_back(Region());
                next_label++;
            }

            regions[labels.at<int>(y, x)].mean_color += img.at<Vec3b>(y, x);
            regions[labels.at<int>(y, x)].pixel_count++;
        }
    }

    for (auto& r : regions) {
        if (r.pixel_count > 0)
            r.mean_color /= r.pixel_count;
    }

    cached_results.push_back(img.clone());

    // merge iteration
    int iter_max = 30;
    for (int iter = 1; iter <= iter_max; iter++) {

        // build adjacency
        int region_num = regions.size();
        vector<set<int>> adjacency(region_num);
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                if (x + 1 < cols) {
                    if (labels.at<int>(y, x) != labels.at<int>(y, x + 1)) {
                        adjacency[labels.at<int>(y, x)].insert(labels.at<int>(y, x + 1));
                        adjacency[labels.at<int>(y, x + 1)].insert(labels.at<int>(y, x));
                    }
                }
                if (y + 1 < rows) {
                    if (labels.at<int>(y, x) != labels.at<int>(y + 1, x)) {
                        adjacency[labels.at<int>(y, x)].insert(labels.at<int>(y + 1, x));
                        adjacency[labels.at<int>(y + 1, x)].insert(labels.at<int>(y, x));
                    }
                }
            }
        }

        // find closest neighbour
        vector<int> merge_to(region_num, -1);
        for (int i = 0; i < region_num; i++) {
            if (regions[i].pixel_count == 0) continue;

            float minDist = FLT_MAX;
            int best = -1;

            for (int j : adjacency[i]) {
                if (regions[j].pixel_count == 0) continue;
                float d = colorDist(regions[i].mean_color, regions[j].mean_color);
                if (d < minDist) {
                    minDist = d;
                    best = j;
                }
            }
            merge_to[i] = best;
        }

        // relabel
        vector<int> new_label(region_num);
        for (int i = 0; i < region_num; i++)
            new_label[i] = (merge_to[i] >= 0) ? min(i, merge_to[i]) : i;

        for (int y = 0; y < rows; y++)
            for (int x = 0; x < cols; x++)
                labels.at<int>(y, x) = new_label[labels.at<int>(y, x)];

        // rebuild regions
        vector<Region> new_regions(region_num);
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                Region& r = new_regions[labels.at<int>(y, x)];
                r.mean_color += img.at<Vec3b>(y, x);
                r.pixel_count++;
            }
        }

        for (auto& r : new_regions) {
            if (r.pixel_count > 0)
                r.mean_color /= r.pixel_count;
        }

        regions = new_regions;

        // output result
        Mat out(rows, cols, CV_8UC3);
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                Vec3f c = regions[labels.at<int>(y, x)].mean_color;
                out.at<Vec3b>(y, x) = Vec3b((uchar)c[0], (uchar)c[1], (uchar)c[2]);
            }
        }

        cached_results.push_back(out);
        cout << "Merge iteration " << iter << " done." << endl;
    }

    namedWindow("Region Unification", WINDOW_AUTOSIZE);
    createTrackbar("merge", "Region Unification", &merge_level, iter_max, onTrackbar);
    onTrackbar(0, 0);

    cout << endl;
    cout << "================Calculation Finished================" << endl;
    cout << "Press s to save current result" << endl;
    cout << "Press ESC to quit program" << endl;

    while (true) {
        int key = waitKey(0);

        if (key == 27) { // press ESC to quit program
            break;
        }
        else if (key == 's' || key == 'S') {
            // save current region-merged image
            string filename = "region_unification_merge_" + to_string(merge_level) + ".png";
            imwrite(filename, cached_results[merge_level]);
            cout << "Saved: " << filename << endl;
        }
    }

    return 0;
}