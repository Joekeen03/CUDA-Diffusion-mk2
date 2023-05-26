#ifndef IMAGE_RENDERER
#define IMAGE_RENDERER

#include <memory>
#include <mutex>

#include <gl/glew.h>

namespace Rendering {
        class ImageBuffer { // Represents a grayscale texture
        public:
            const int width;
            const int height;
            const int nBytes;
            ImageBuffer(const int widthArg, const int heightArg);
            // Copy nBytes from source to imageBuffer
            void WriteRGBAImage(GLubyte *source);
            // Copy nBytes/4 from source to imageBuffer;
            void WriteGrayScaleImage(GLubyte *source);
            void ReadImage(GLubyte *dest);
        protected:
            // Not entirely good practice, having a mutex-protected variable? Since a derived class could
            //  fail to respect the mutex (though, the base class could too...)...I think a better expression
            //  of my concern is that the base class, defining the mutex-protected variable, should be responsible
            //  for managing the mutex. And if the variable is accessible to the derived class, any
            //  derived class also becomes responsible for managing the mutex. But at the same time, this
            //  allows the derived class to do stuff with the variable beyond what the base class can
            //  do - extending the base class as a derived class should.
            const std::unique_ptr<GLubyte[]> imageBuffer;
            std::mutex imageMutex;
            bool updated;
    };
    bool Setup(int argc, char** argv, const int width, const int height);
    std::shared_ptr<ImageBuffer> FetchBuffer();
    int Run();
}

#endif //IMAGE_RENDERER