#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include "NativeLogging.h"

static const char* TAG = "CreatePano";

using namespace std;
using namespace cv;

void find_correspondences(vector<Mat> inputImages, vector<vector<Point2f>>& imgMatches, string outputImagePath, bool writeOutput)
{
    // Extract features from each picture with your favorite detector.

    // Match the features.

    // Prune 'bad' matches based on some metric.

	// Find the point correspondences.

    /* Visualize and report your 'good_matches' (if enabled). */
    if(writeOutput)
    {
      Mat intermediateImage;
      // Draw your matches here
      imwrite(outputImagePath, intermediateImage);
    }
}

Mat apply_homography(vector<Mat>& inputImages, vector<vector<Point2f>>& matches)
{

    /* **************************************************************
     * If you got this far, congrats!! You are half way there. Now for the second portion.
     * *************************************************************** */

    // Re-comment out the drawMatches block.

    // Compute the homography via RANSAC.

    // Compute the corner corrospondences.

    Mat warpedImage;
    // Create the left and right templates (warpPerspective might be useful here).
    return warpedImage;
}

Mat blend_images(vector<Mat>& inputImages, Mat& warpedImage)
{

    // Create the blending weight matrix, make sure the white portion is the right size! This matters!

    // Implement Laplacian blending.

	// Play with the number of levels when finished.
    Mat outputPano;

    // Create your output image here.

    return outputPano;
}

bool CreatePanorama(vector<string> inputImagePaths, string outputImagePath)
{

    /*  We have broken this development into two stages.
     *
     *  The steps in the first portion are as follows:
     *   1) Extract features from each picture (Helpful OpenCV functions: OrbFeatureDetector)
     *   2) Match the features                 (Helpful OpenCV functions: BFMatcher)
     *   3) Prune the matches                  (Helpful OpenCV functions: DMatch)
     *   4) Output the drawMatches             (Helpful OpenCV functions: drawMatches)

     *  The steps in the second portion are as follows:
     *   1) Compute the homography via RANSAC  (Helpful OpenCV functions: findHomography, perspectiveTransform)
     *   2) Create the left and right templates(Helpful OpenCV functions: Mat())
     *   3) Create the blendingWeights         (Helpful OpenCV functions: Range::all())
     *   4) Implement Laplacian blending       (Helpful OpenCV functions: pyrUp/pyrDown)
     */
    vector<Mat> inputImages;
    for(string imgPath : inputImagePaths)
    {
        Mat img = imread(imgPath);
        inputImages.push_back(img);
    }

    // Set this to true if you want to visualize your intermediate matches.
    bool saveIntermediate = false;
    vector<vector<Point2f>> imgMatches(2);
    find_correspondences(inputImages, imgMatches, outputImagePath, saveIntermediate);
    if(saveIntermediate)
    {
    	return true;
    }

    Mat warpedImage = apply_homography(inputImages, imgMatches);

    Mat outputPano = blend_images(inputImages, warpedImage);

    imwrite(outputImagePath, outputPano);

    return true;
}

