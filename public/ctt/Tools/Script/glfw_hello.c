//gcc -o glfw_hello glfw_hello.c -L/usr/local/lib -lglfw -I/usr/local/include -framework OpenGL -framework GLUT


#include <GLFW/glfw3.h>

int main(int argc, char** argv)
{
    if (!glfwInit()){
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(512, 384, "Hello World", NULL, NULL);
    if (!window){
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    while (!glfwWindowShouldClose(window)){
        
        glBegin(GL_TRIANGLES);
        glColor3f(1.0, 0.0, 0.0);    // Red
        glVertex3f(0.0, 0.5, 0.0);
        glColor3f(0.0, 1.0, 0.0);    // Green
        glVertex3f(-0.5, -0.5, 0.0);
        glColor3f(0.0, 0.0, 1.0);    // Blue
        glVertex3f(0.5, -0.5, 0.0);
        glEnd();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
