#include "imageRenderer.h"

#include <memory>
#include <mutex>
#include <iostream>
#include <fstream>
#include <string>

#include <gl/glew.h>
#include <GL/freeglut.h>

namespace Rendering {
    const std::string vertexShaderPath = ".\\build\\shaders\\vertexShader.vs";
    const std::string fragmentShaderPath = "./build/shaders/fragmentShader.fs";

    constexpr int infoLogSize = 1024;

    
    ImageBuffer::ImageBuffer(const int widthArg, const int heightArg)
                : width(widthArg), height(heightArg), nBytes(width*height*4),
                    imageBuffer(std::make_unique<GLubyte[]>(nBytes)),
                    updated(true) {} 
    // Copy nBytes from source to imageBuffer
    void ImageBuffer::WriteRGBAImage(GLubyte *source) {
        std::lock_guard<std::mutex> guard(imageMutex);
        std::memcpy(imageBuffer.get(), source, nBytes);
        updated = true;
    }
    // Copy nBytes/4 from source to imageBuffer;
    void ImageBuffer::WriteGrayScaleImage(GLubyte *source) {
        std::lock_guard<std::mutex> guard(imageMutex);
        for (size_t i = 0; i < nBytes; i++) {
            imageBuffer[i] = source[i/4];
        }
        
        updated = true;
    }

    void ImageBuffer::ReadImage(GLubyte *dest) {
        std::lock_guard<std::mutex> guard(imageMutex);
        std::memcpy(dest, imageBuffer.get(), nBytes);
    }

    class TextureBuffer: public ImageBuffer { // Represents a grayscale texture
        public:
            TextureBuffer(const int widthArg, const int heightArg) : ImageBuffer(widthArg, heightArg) {
                glGenTextures(1, &texture);
                if (texture == 0) {
                    fprintf(stderr, "Error binding texture.");
                    exit(-1);
                }
                glBindTexture(GL_TEXTURE_2D, texture);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }

            void UpdateAndBind(GLenum textureUnit) {
                std::lock_guard<std::mutex> guard(imageMutex);
                glBindTexture(GL_TEXTURE_2D, texture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageBuffer.get()); // update
                glActiveTexture(textureUnit);
                glBindTexture(GL_TEXTURE_2D, texture);
                updated = false;
            }
        private:
            GLuint texture;
    };

    class Vector3fc {
        public:
            const float x;
            const float y;
            const float z;
            Vector3fc(float xArg, float yArg, float zArg) : x(xArg), y(yArg), z(zArg) {}
    };

    class Vector2fc {
        public:
            const float x;
            const float y;
            Vector2fc(float xArg, float yArg) : x(xArg), y(yArg) {}
    };

    class Vertex {
        public:
            const Vector3fc vector;
            const Vector2fc textureCoords;
            Vertex(Vector3fc vectorArg, Vector2fc textureCoordsArg) : vector(vectorArg), textureCoords(textureCoordsArg) {}
    };
    GLuint VBO;
    std::shared_ptr<TextureBuffer> texture;
    int frame = 0;
    std::unique_ptr<GLubyte[]> data;

    void SetupBuffer() {
        Vertex corners[] = {
            Vertex(Vector3fc(-1.0f, -1.0f, 0.0f), Vector2fc(0.0f, 0.0f)),
            Vertex(Vector3fc(-1.0f, 1.0f, 0.0f), Vector2fc(0.0f, 1.0f)),
            Vertex(Vector3fc(1.0f, -1.0f, 0.0f), Vector2fc(1.0f, 0.0f)),
            Vertex(Vector3fc(1.0f, 1.0f, 0.0f), Vector2fc(1.0f, 1.0f)),
        };

        Vertex vertices[] = {
            corners[0], corners[1], corners[2],
            corners[1], corners[2], corners[3]
        };

        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    }

    bool ReadFile(const std::string fileName, std::string &destination) {
        std::ifstream fileObject(fileName);
        if (fileObject.is_open()) {
            std::string buffer;
            while (getline(fileObject, buffer)) {
                destination += buffer+'\n';
            }
            fileObject.close();
            return true;
        }
        return false;
    }

    void CreateShader(const GLenum shaderType, const std::string shaderSourceFileName, const GLuint program) {
        GLuint shader = glCreateShader(shaderType);

        std::string shaderText;
        
        if (!ReadFile(shaderSourceFileName, shaderText)) {
            fprintf(stderr, "Error reading shader source file: '%s'\n", shaderSourceFileName.c_str());
            exit(-1);
        }
        
        const char* rawShaderText = shaderText.c_str();
        const GLchar* textPointer[] = {rawShaderText};
        GLint lengths[] = {(GLint)shaderText.length()};
        glShaderSource(shader, 1, textPointer, lengths);

        glCompileShader(shader);

        GLint success = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (success == 0) {
            GLchar infoLog[infoLogSize];
            glGetShaderInfoLog(shader, infoLogSize, NULL, infoLog);
            fprintf(stderr, "Error compiling shader type %d: '%s'\n", shaderType, infoLog);
            exit(-1);
        }

        glAttachShader(program, shader);
    }

    GLuint CreateShaderProgram() {
        GLuint shaderProgram = glCreateProgram();
        if (shaderProgram == 0) {
            fprintf(stderr, "Error creating shader program.\n");
            exit(-1);
        }

        CreateShader(GL_VERTEX_SHADER, vertexShaderPath, shaderProgram);
        CreateShader(GL_FRAGMENT_SHADER, fragmentShaderPath, shaderProgram);

        GLint success = 0;
        GLchar errorLog[infoLogSize] = {0};

        glLinkProgram(shaderProgram);
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (success == 0) {
            glGetProgramInfoLog(shaderProgram, infoLogSize, NULL, errorLog);
            fprintf(stderr, "Error linking shader program: '%s'\n", errorLog);
            exit(-1);
        }

        glValidateProgram(shaderProgram);
        glGetProgramiv(shaderProgram, GL_VALIDATE_STATUS, &success);
        if (success == 0) {
            glGetProgramInfoLog(shaderProgram, infoLogSize, NULL, errorLog);
            fprintf(stderr, "Invalid shader program: '%s'\n", errorLog);
            exit(-1);
        }
        return shaderProgram;
    }

    void SetupShader() {
        GLuint shaderProgram = Rendering::CreateShaderProgram();
        glUseProgram(shaderProgram);
        
        GLint samplerLocation = glGetUniformLocation(shaderProgram, "sampler");
        glUniform1f(samplerLocation, 0);
    }

    void UpdateData() {
        for (int i = 0; i < texture->nBytes; i++) {
            data[i] = (i+frame)%256;
        }
        texture->WriteRGBAImage(data.get());
    }

    void InitializeTexture(const unsigned int width, const unsigned int height) {
        texture = std::make_shared<TextureBuffer>(width, height);
        data = std::make_unique<GLubyte[]>(texture->nBytes);
        UpdateData();
        texture->UpdateAndBind(GL_TEXTURE0);
    }

    void RenderSceneCB() {
        UpdateData();

        glClear(GL_COLOR_BUFFER_BIT);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)sizeof(Vector3fc));
        texture->UpdateAndBind(GL_TEXTURE0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(0);
        glutPostRedisplay();
        glutSwapBuffers();
        frame++;
    }

    bool Setup(int argc, char** argv, const int width, const int height) {
        glutInit(&argc, argv);
        glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH);

        glutInitWindowSize(width, height);

        int x = 200;
        int y = 0;
        glutInitWindowPosition(x, y);

        int windowID = glutCreateWindow("Image Renderer");
        GLclampf red = 0.0f, green = 0.0f, blue = 0.0f, alpha = 0.0f;
        glClearColor(red, green, blue, alpha);

        glutDisplayFunc(Rendering::RenderSceneCB);

        // OpenGL setup:
        GLenum result = glewInit();
        if (result != GLEW_OK) {
            fprintf(stderr, "Failed to initialize GLEW: '%s'\n", glewGetErrorString(result));
            return false;
        }

        SetupBuffer();
        SetupShader();
        InitializeTexture(width, height);
        return true;
    }

    std::shared_ptr<ImageBuffer> FetchBuffer() {
        return texture;
    }

    int Run() {
        glutMainLoop();
        return 0;
    }
}