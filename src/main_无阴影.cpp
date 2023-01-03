#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <math.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);

const unsigned int SCR_WIDTH = 800; 
const unsigned int SCR_HEIGHT = 600;

// 光源在世界坐标的位置（平移向量）
glm::vec3 lightPos(0.5f, 1.0f, 2.0f);

// 这是一个顶点着色器的配置，是一个C语言风格的着色器语言
const char *vertexShaderSource = "#version 330 core\n"	//这个地方是3.3版本核心模式，所以设置为330core即可
								 "layout (location = 0) in vec3 aPos;\n"
								 "layout (location = 1) in vec2 aTexCoord;\n"
								 "layout (location = 2) in vec3 aNormalVec;\n"
								//  "out vec3 ourColor;\n"	//顶点着色器需要输出的是ourColor，用于下面片段着色器接收
								 "out vec2 TexCoord;\n"		// 传给片段着色器纹理坐标
								 "out vec3 FragPosition;\n"		// 计算并传给片段着色器，该片段所在的世界坐标
								 "out vec3 NormalVec;\n"	// 传给片段着色器法线方向
								 "uniform mat4 model;\n"
								 "uniform mat4 view;\n"
								 "uniform mat4 projection;\n"
								//  "uniform mat4 transform;\n"	// 变换矩阵
								 "void main()\n"
								 "{\n"
								 " gl_Position = projection * view * model * vec4(aPos.x, aPos.y, aPos.z, 1.0f);\n"	//顶点着色器首先要传出去的必须是位置属性
									//这里我们还要注意的是，这个地方还没有乘上如model-view-projection矩阵。
									//如果有需要，需要乘这个矩阵。 另外，我们将vec3再加上1，是为了形成四元表示
								 " TexCoord = vec2(aTexCoord.x, aTexCoord.y);\n"	// 为了有纹理图案
								 " FragPosition = vec3(model * vec4(aPos, 1.0f));\n"	// 有了顶点坐标，那么根据该顶点的四元坐标进行Model变换即可得到世界坐标系下的片段坐标
								//  " NormalVec = aNormalVec;\n"
								 " NormalVec = vec3(transpose(inverse(model)) * vec4(aNormalVec, 1.0f));\n"	// 传递给片段着色器予以处理漫反射光照
								 // 因为仅包含平移和旋转，这种条件下可以根据法线矩阵定理使用model的逆的转置并取其前3*3子矩阵进行操作
								 "}\0";

// 片段着色器用来指定我们最后成色是什么颜色的着色器。
const char *fragmentShaderSource = "#version 330 core\n"
								   "out vec4 FragColor;\n"	// 片元的着色结果
								   "in vec2 TexCoord;\n"	// 传入由顶点着色器传出的纹理坐标
								   "in vec3 FragPosition;\n"
								   "in vec3 NormalVec;\n"	// 传入由顶点着色器得到的各个顶点的法向量
									"uniform vec3 objectColor;\n"
									"uniform vec3 lightColor;\n "	
									"uniform sampler2D ourTexture;\n"	// 二维纹理采样器
									"uniform vec3 lightPosition\n;"	// 光源的坐标
									"uniform vec3 viewPosition\n;"	// 视角的世界坐标位置
								   "void main()\n"
								   "{\n"
								    //  环境光光照ambient
									//  定义环境光强度为0.1，较小的值以模拟环境的微光
								   " float ambientStrength = 0.1;\n"
									" vec3 ambient = ambientStrength * lightColor;\n"

									//  漫反射光照diffuse：取决于法向+光向
									//  首先计算片段到光源的方向 与 片段表面的法向量的夹角
									" vec3 normal_dir = normalize(NormalVec);\n"	// 对法线方向进行标准化
									" vec3 light_dir = normalize(lightPosition - FragPosition);\n"	// 从片元到发光点的向量标准化
										// 用单位向量点积来表示漫反射强弱，范围为[-1,1]
										// 如果点积结果为负，表示光线本应该照不到片段表面，应该舍弃
									" float diffStrength = max(dot(normal_dir, light_dir), 0.0f);\n"
									" vec3 diffuse = diffStrength * lightColor;\n"


									//  镜面反射高光specular: 取决于法向+光向+视角方向，视角与反射光线角度越小则光强度越大
									//  首先给定镜面反射常量
									" float specularStrengthConst = 1.0f;\n"
									" int Shininess = 10;\n"	// 高光反光度；越大，则散射少，高光点形状小。
									//  计算片段到的视角方向
									" vec3 view_dir = normalize(viewPosition - FragPosition);\n"
									//  计算反射光线的方向,reflect第一个参数：从光源指向片段的向量，利用光线方向取负号。
									//  第二个参数是法线方向
									" vec3 reflect_dir = reflect(-light_dir, normal_dir);\n"
									" vec3 halfway_dir = normalize(light_dir + view_dir);\n"

									//  普通冯氏光照
									// " float specularStrength = specularStrengthConst * pow(max(dot(view_dir, reflect_dir), 0.0f), Shininess);\n"
									//  blinn-phong光照
									" float specularStrength = specularStrengthConst * pow(max(dot(view_dir, halfway_dir), 0.0f), Shininess);\n"

									" vec3 specular = specularStrength * lightColor;\n"	// 镜面高光


									// 将三个光源的光相加得到总光源
									" vec3 result = (ambient + diffuse + specular);"
									// 计算总光照下的纹理显示
									" FragColor = vec4(result, 1.0f) * texture(ourTexture, TexCoord) * vec4(objectColor, 1.0f);\n"	// 使用GLSL内建的texture函数来采样纹理的颜色，它第一个参数是纹理采样器，第二个参数是对应的纹理坐标。
									
									
									
									// texture函数会使用之前设置的纹理参数对相应的颜色值进行采样，这个片段着色器的输出就是纹理的（插值）纹理坐标上的（过滤后的颜色）
								//    " FragColor = vec4(ourColor, 1.0f);\n"//这个是我们要传输出去的向量，这个决定了我们这个片段的颜色是什么
								   "}\n\0";


// 深度缓冲着色器：将顶点渲染到以光源为camera视角的着色器
const char *depthVertexShaderSource = "#version 330 core\n"
										"layout (location = 0) in vec3 position;\n"
										"uniform mat4 lightSpaceMatrix;\n"
										"uniform mat4 model;\n"
										"void main()\n"
										"{\n"
										"gl_Position = lightSpaceMatrix * model * vec4(position, 1.0f);\n"
										"}\n\0";

const char *depthFragmentShaderSource = "#version 330 core\n"
										"void main()\n"
										"{\n"
										"}\n\0";

//	光源的顶点着色器
const char *lightVertexShaderSource = "#version 330 core\n"
									"layout (location = 0) in vec3 aPos;\n"
									"uniform mat4 model;\n"
									"uniform mat4 view;\n"
									"uniform mat4 projection;\n"
									"void main()\n"
									"{\n"
									"	gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
									"}\n\0";

// 光源的片段着色器
const char *lightFragmentShaderSource = "#version 330 core\n"
								"out vec4 FragColor;\n"
								"void main()\n"
								"{\n"
								" FragColor = vec4(1.0);\n"
								"}\n\0";


int main()
{
	// glfw: initialize and configure
	// 可以定义opengl中的参数
	// ------------------------------
	
	// 首先初始化
	glfwInit();
	// 主版本号
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	// 次版本号
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// 设置：我们使用的是核心的模式，意味着我们只能使用opengl功能的一个子集
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

// #ifdef __APPLE__
// 	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
// #endif

	// glfw window creation
	// --------------------
	// 这里需要输入参数，窗口的宽和高
	// 返回的这个是OpenGLWindow窗口对象，这个窗口对象存放了所有和窗口相关的数据，而且会被GLFW的其他函数频繁地用到
	GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "纹理+坐标变换+blin-phong光照效果", NULL, NULL);

	if (window==NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	// 创建完窗口之后，就可以通知glfw把我们的上下文设置为当前线程的主上下文
	glfwMakeContextCurrent(window); 
	// 回调函数的作用：每次我们可能都会改变我们的窗口大小，那么对应的视口需要调整
	// 每当我们窗口被改变的时候就会调用这个函数，然后视口就会作出相应变化
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// glad: load all OpenGL function pointers
	// glad是用来管理OpenGL的函数指针的，所以在调用任何OpenGL的函数之前，我们需要初始化glad
	// 因为openGL知识一个标准/规范，具体的实现是有驱动开发商针对特定显卡实现的。
	// 由于Opengl驱动版本过多，他大多数的函数位置都无法在编译的时候确定下来，需要在运行时查询。所以任函数位置查询
	// 任务就落在了开发者身上
	// 开发者需要在运行时获取函数地址并将其保存在一个函数指针中供以后使用
	//  ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl; 
		return -1;
	}


	//--------------开启z-buffer深度测试-----------------
	glEnable(GL_DEPTH_TEST);



// ----------------------着色器的定义--------------------------------
	// build and compile our shader program
	// ------------------------------------


	// 物体的vertex shader
	// 首先创建一个顶点着色器对象
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER); 
	// 把开头写的源码附加给这个着色器对象
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL); 
	// 直接编译这个着色器对象（因为之前只是C语言的代码）
	glCompileShader(vertexShader);
	// check for shader compile errors 
	// 查看编译是否成功的一段代码
	int success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success); 
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog); 
		std::cout << "1ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}



	// 物体的fragment shader
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER); 
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL); 
	glCompileShader(fragmentShader);
	// check for shader compile errors
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog); 
		std::cout << "1ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}


	// 我们需要获得完整的着色器程序，则需要将顶点着色器与片源着色器链接起来
	// 因为着色器成色对象是多个着色器合并之后最后链接再完成的版本。
	// 物体的link shaders
	unsigned int shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// check for linking errors
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "1ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
				  << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);



	
	// 深度缓冲顶点着色器
	unsigned int depthVertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(depthVertexShader, 1, &depthVertexShaderSource, NULL);
	glCompileShader(depthVertexShader);
	glGetShaderiv(depthVertexShader, GL_COMPILE_STATUS, &success); 
	if (!success)
	{
		glGetShaderInfoLog(depthVertexShader, 512, NULL, infoLog); 
		std::cout << "2ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}



	// 深度缓冲片段着色器
	unsigned int depthFragmentShader = glCreateShader(GL_FRAGMENT_SHADER); 
	glShaderSource(depthFragmentShader, 1, &depthFragmentShaderSource, NULL); 
	glCompileShader(depthFragmentShader);
	// check for shader compile errors
	glGetShaderiv(depthFragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(depthFragmentShader, 512, NULL, infoLog); 
		std::cout << "2ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}

	// 深度缓冲着色器link
	unsigned int depthShaderProgram = glCreateProgram();
	glAttachShader(depthShaderProgram, depthVertexShader);
	glAttachShader(depthShaderProgram, depthFragmentShader);
	glLinkProgram(depthShaderProgram);
	// check for linking errors
	glGetProgramiv(depthShaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(depthShaderProgram, 512, NULL, infoLog);
		std::cout << "3ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
				  << infoLog << std::endl;
	}
	glDeleteShader(depthVertexShader);
	glDeleteShader(depthFragmentShader);


	// 光源的vertex shader
	unsigned int lightVertexShader = glCreateShader(GL_VERTEX_SHADER); 
	glShaderSource(lightVertexShader, 1, &lightVertexShaderSource, NULL); 
	glCompileShader(lightVertexShader);
	glGetShaderiv(lightVertexShader, GL_COMPILE_STATUS, &success); 
	if (!success)
	{
		glGetShaderInfoLog(lightVertexShader, 512, NULL, infoLog); 
		std::cout << "3ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}



	// 光源的fragment shader
	unsigned int lightFragmentShader = glCreateShader(GL_FRAGMENT_SHADER); 
	glShaderSource(lightFragmentShader, 1, &lightFragmentShaderSource, NULL); 
	glCompileShader(lightFragmentShader);
	// check for shader compile errors
	glGetShaderiv(lightFragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(lightFragmentShader, 512, NULL, infoLog); 
		std::cout << "3ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}

	// 光源的link Shader
	unsigned int lightShaderProgram = glCreateProgram();
	glAttachShader(lightShaderProgram, lightVertexShader);
	glAttachShader(lightShaderProgram, lightFragmentShader);
	glLinkProgram(lightShaderProgram);
	// check for linking errors
	glGetProgramiv(lightShaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(lightShaderProgram, 512, NULL, infoLog);
		std::cout << "3ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
				  << infoLog << std::endl;
	}
	glDeleteShader(lightVertexShader);
	glDeleteShader(lightFragmentShader);




// ------------渲染流水线-----------------------
	// 一个立方体
	GLfloat vertices[] = {
	// 后面
	// -----位置-------   --纹理坐标-- ---手工指定的法向------
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  0.0f,  0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f,  0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  0.0f,  0.0f, -1.0f,

	// 前面
    -0.5f, -0.5f, 0.5f,  0.0f, 0.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f, 0.5f,  1.0f, 0.0f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f, 0.5f,  1.0f, 1.0f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f, 0.5f,  1.0f, 1.0f,  0.0f,  0.0f, 1.0f,
    -0.5f,  0.5f, 0.5f,  0.0f, 1.0f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f, 0.5f,  0.0f, 0.0f,  0.0f,  0.0f, 1.0f,

	// 左面
    -0.5f,  0.5f, 0.5f,  1.0f, 0.0f,   -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, 0.5f,  1.0f, 0.0f,   -1.0f,  0.0f,  0.0f,

	// 右面
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  1.0f,  0.0f,  0.0f,

	// 下面
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  0.0f, -1.0f,  0.0f,

	// 上面
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  0.0f,  1.0f,  0.0f
	};

	glm::vec3 cubePositions[] = {
		glm::vec3( 0.0f,  0.0f,  0.0f), 
		glm::vec3( 2.0f,  5.0f, -15.0f), 
		glm::vec3(-1.5f, -2.2f, -2.5f),  
		glm::vec3(-3.8f, -2.0f, -12.3f),  
		glm::vec3( 2.4f, -0.4f, -3.5f),  
		glm::vec3(-1.7f,  3.0f, -7.5f),  
		glm::vec3( 1.3f, -2.0f, -2.5f),  
		glm::vec3( 1.5f,  2.0f, -2.5f), 
		glm::vec3( 1.5f,  0.2f, -1.5f), 
		glm::vec3(-1.3f,  1.0f, -1.5f)
	};


	unsigned int VBO, VAO;	// VAO作用：本身不存储顶点数据，顶点数据是存在VBO中的，其实对很多VBO的引用
	// VAO相当于是对很多个VBO的引用，把一些VBO组合在一起作为一个对象统一管理。
	// 先生成VAO，然后生成的VBO都会在VAO的管控之下
	// 用于顶点数组对象的产生
	glGenVertexArrays(1, &VAO); 
	// 创建/调用缓冲区，第一个参数：生成的缓冲区数量；第二个参数：用于存储单个id或多个id的GLuint变量或数组的地址
	// 1：缓冲区数量；&VBO：把缓冲区数据放的位置
	glGenBuffers(1, &VBO);
	// 首先绑定 Vertex Array Object, 然后绑定和设定 vertex buffer(s), 
	// 再配置 vertex attributes(s).顶点属性
	glBindVertexArray(VAO);




	// 绑定顶点对象的缓冲区，绑定到显存的缓冲区
	// 第一个参数就是告诉OpenGL这个缓存对象是要干什么的
	// 第二个参数就是要传入缓存区的对象
	// 这里的含义就是告诉它，我们的VBO这片缓冲区实际就是数组缓冲区
	// 这片缓冲区可以存各种位置、各种颜色、各种纹理坐标
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// 这一步是把我们要缓存的数据传进缓冲区中，第一个参数就是上面的具体的GL_ARRAY_BUFFER
	// 第二个参数是传入的字节数量， 第三个参数是原数据内容数组的指针
	// 第四个参数是缓冲区对象的使用方式，这是一个性能提示，帮助OpenGL在正确的位置去分配内存的。
	// 当我们的数据几乎不会改变的时候，就对它进行GL_STATIC_DRAW的存储
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// position attribute位置属性
	// 链接顶点属性：第一个参数：0是顶点着色器的layout定义的location=0；第二个参数：3是顶点属性的大小，因为vector是3，所以大小是3
	// 第三个参数指定数据的类型。第四个表示是否需要标准化，如果为GL_TRUE则数据映射到0和1之间，我们这里不需要所以直接设置为GL_FALSE即可
	// 第五个参数为步长，告诉我们在连续顶点属性组之间的的间隔。由于下一个组的位置在6个float之后，所以直接把步长设为6*sizeof(float)
	// 最后一个参数是代表数据在缓冲位置中的起始位置的偏移量。比如下面的颜色属性配置的时候，起始位置是三个值之后，所以。。
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// // color attribute
	// glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) (3 * sizeof(float)));
	// glEnableVertexAttribArray(1);

	// 顶点的纹理坐标属性
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *)(3*sizeof(float)));
	glEnableVertexAttribArray(1);

	// 顶点的法线方向属性
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *)(5*sizeof(float)));
	glEnableVertexAttribArray(2);



	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't
	// unbind VAOs (nor VBOs) when it's not directly necessary.
	// glBindVertexArray(0);


	
	// ------------------------构建光源VAO---------------------------------
	unsigned int lightVAO;
	glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);





	// --------------------正方体纹理------------------------------------------
	// 产生纹理对象(也是通过ID引用的！)
	unsigned int texture;
	glGenTextures(1, &texture);	//  参数一：需要生成纹理的数量；参数二：指定生成的纹理保存在哪里
	// 绑定纹理对象
	glActiveTexture(GL_TEXTURE0); // 在绑定纹理之前先激活纹理单元
	glBindTexture(GL_TEXTURE_2D, texture);	//让之后任何的纹理指令都可以配置当前绑定的纹理

	// float borderColor[] = { 1.0f, 1.0f, 0.0f, 1.0f };
	// glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	
	// 为当前绑定的纹理对象设置环绕、过滤方式
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);   
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);	// TODO:最后一个参数可能需要改成GL_LINEAR_MIPMAP_LINEAR以实现多级渐远纹理
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// 纹理图片导入
	int width, height, nrChannels;
	unsigned char *data = stbi_load("merry_christmas_mr_Lawrence.jpg", &width, &height, &nrChannels, 0);
	// stbi_set_flip_vertically_on_load(true);
	// 生成纹理
	if (data)
	{// 如果load成功了texture
		// 现在纹理已经绑定了，我们可以使用前面载入的图片数据生成一个纹理了。纹理可以通过glTexImage2D来生成：
		// para1:指定纹理目标，设置为2D意味着会生成与当前绑定的文理对象在同一个目标上的纹理
		// para2:为纹理指定多级渐远纹理的级别。可以单独手动设置多个级别。0为基本级别
		// para3:告诉OpenGL我们希望把纹理处理为什么格式。
		// para4、5:设置最终的纹理宽度和高度。由于之前加载图像的时候就保存了它们，故使用对应的变量
		// para6:总是设为0；para7、8:定义了源图的格式和数据类型，我们使用RGB值加载这个图像，并把它们存储为char(byte)数组，我们将会传入对应的值
		// 最后一个参数是真正的图像数据。
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		// 下面这个函数可以在生成纹理后调用，作用是为当前绑定的纹理自动生成所有需要的多级渐远纹理
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	// 生成了纹理和相应的多级渐远纹理后，释放图像的内存
	stbi_image_free(data);


	// -----------------------变换-----------------------------
	// 1.模型变换矩阵
	glm::mat4 model;
	model = glm::rotate(model, glm::radians(-55.0f), glm::vec3(1.0f, 0.0f, 0.0f));	// 这个长方形绕着x轴旋转55度

	// 2.观察矩阵
	// glm::mat4 view;
	// 将摄像机向后移动，和将整个场景向前移动是一样的。我们想要退后来观察，等价于让物体向前走两步。由于z轴正轴指向屏幕以外，所以平移时z值为负
	// view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

	// 3.投影矩阵
	glm::mat4 projection;
	projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);// 第一个参数通常设置为45.0f，以达到真实效果；第二个参数为屏幕的宽高比；第三、四个参数表示近远平面的z距离。


















// -------------------------------------------地板----------------------------------------
	GLfloat planeVertices[] = {
        //---Positions-----     //---Normals-----   //-Texture Coords-
         25.0f, -3.5f,  25.0f,	 0.0f, 1.0f, 0.0f,	25.0f, 0.0f,
        -25.0f, -3.5f, -25.0f,	 0.0f, 1.0f, 0.0f, 	0.0f, 25.0f,
        -25.0f, -3.5f, 25.0f,	 0.0f, 1.0f, 0.0f, 	0.0f, 0.0f,

         25.0f, -3.5f,  25.0f,	 0.0f, 1.0f, 0.0f, 	25.0f, 0.0f,
         25.0f, -3.5f, -25.0f,	 0.0f, 1.0f, 0.0f, 	25.0f, 25.0f,
        -25.0f, -3.5f, -25.0f,	 0.0f, 1.0f, 0.0f, 	0.0f, 25.0f
    };
    GLuint planeVAO, planeVBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), &planeVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
    glBindVertexArray(0);

	//-------------------------------产生地板纹理------------------------------
	unsigned int texture_floor;
	glGenTextures(1, &texture_floor);	//  参数一：需要生成纹理的数量；参数二：指定生成的纹理保存在哪里
	// 绑定纹理对象
	glActiveTexture(GL_TEXTURE1); // 在绑定纹理之前先激活纹理单元
	glBindTexture(GL_TEXTURE_2D, texture_floor);	//让之后任何的纹理指令都可以配置当前绑定的纹理

	// float borderColor_1[] = { 1.0f, 1.0f, 0.0f, 1.0f };
	// glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor_1);

	
	// 为当前绑定的纹理对象设置环绕、过滤方式
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);   
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	// TODO:最后一个参数可能需要改成GL_LINEAR_MIPMAP_LINEAR以实现多级渐远纹理
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// float borderColor[] = { 1.0f, 1.0f, 0.0f, 1.0f };
	// glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	// 纹理图片导入
	int width_1, height_1, nrChannels_1;
	// unsigned char *data_1 = stbi_load("merry_christmas_mr_Lawrence.jpg", &width_1, &height_1, &nrChannels_1, 0);
	unsigned char *data_1 = stbi_load("floor.jpg", &width_1, &height_1, &nrChannels_1, 0);
	// stbi_set_flip_vertically_on_load(true);
	// 生成纹理
	if (data_1)
	{// 如果load成功了texture
		// 现在纹理已经绑定了，我们可以使用前面载入的图片数据生成一个纹理了。纹理可以通过glTexImage2D来生成：
		// para1:指定纹理目标，设置为2D意味着会生成与当前绑定的文理对象在同一个目标上的纹理
		// para2:为纹理指定多级渐远纹理的级别。可以单独手动设置多个级别。0为基本级别
		// para3:告诉OpenGL我们希望把纹理处理为什么格式。
		// para4、5:设置最终的纹理宽度和高度。由于之前加载图像的时候就保存了它们，故使用对应的变量
		// para6:总是设为0；para7、8:定义了源图的格式和数据类型，我们使用RGB值加载这个图像，并把它们存储为char(byte)数组，我们将会传入对应的值
		// 最后一个参数是真正的图像数据。
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width_1, height_1, 0, GL_RGB, GL_UNSIGNED_BYTE, data_1);
		// 下面这个函数可以在生成纹理后调用，作用是为当前绑定的纹理自动生成所有需要的多级渐远纹理
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	// 生成了纹理和相应的多级渐远纹理后，释放图像的内存
	stbi_image_free(data_1);



// ------------------------------------深度映射FBO----------------------------------------------
	// 为渲染的深度贴图创建一个帧缓冲对象
	GLuint depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);
	// 创建一个2D纹理，提供给帧缓冲的深度缓冲使用
	const GLuint SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	GLuint depthMap;	// 2D纹理对象，深度映射
	glGenTextures(1, &depthMap);
	glActiveTexture(GL_TEXTURE2); // 在绑定纹理之前先激活纹理单元
	glBindTexture(GL_TEXTURE_2D, depthMap);	// 绑定纹理对象

	// ***核心：因为只关心深度值，所以纹理格式要设定为GL_DEPTH_COMPONENT
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, // 1.目标为2D贴图；2.纹理格式要设定为GL_DEPTH_COMPONENT；3&4.阴影纹理图像本身的宽高：表示深度贴图的分辨率
	// 5.纹理格式要设定为GL_DEPTH_COMPONENT， 6.数据类型为float
				SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);	// 近邻过滤
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// 将上面生成的深度纹理 作为 帧缓冲的深度缓冲
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);	// 绑定帧缓冲对象到指定位置
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);	
	glDrawBuffer(GL_NONE);	// 读缓冲：显式地告诉OpenGL不去渲染颜色数据
	glReadBuffer(GL_NONE);	// 绘制缓冲：不去绘制颜色
	glBindFramebuffer(GL_FRAMEBUFFER, 0);








// -----------------------------------------------------渲染循环--------------------------------------------------------
	// -----------
	while (!glfwWindowShouldClose(window))
	{


		// input
		// -----
		processInput(window);

		// render
		// ------
		// 首先改变背景的颜色，清除掉这三个颜色
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		// 清除颜色、深度信息
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// ----------------------------其次，画物体+地板------------------------------------------
		// 激活物体的着色器
		glUseProgram(shaderProgram);
		
		
		// 片段着色器objectColor颜色随时间变化：
		GLfloat timeValue = glfwGetTime();
		GLfloat greenValue = (sin(timeValue) / 2) + 0.5;
		GLfloat redValue = (cos(timeValue) / 2) + 0.5;
		GLfloat blueValue = (tan(timeValue));


		//				------调整 物体和地板 共用的变量------
		// 1.灯光的颜色（片段着色器）
		GLint lightColorLocation = glGetUniformLocation(shaderProgram, "lightColor");
		glUniform3f(lightColorLocation, 1.0f, 1.0f, 1.0f);	// 白色光源

		// 2.view矩阵/相机根据输入交互进行调整位置 + viewPosition（片段着色器）
		glm::mat4 view = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
        float radius = 10.0f;
        float camX = static_cast<float>(sin(glfwGetTime()) * radius);
        float camZ = static_cast<float>(cos(glfwGetTime()) * radius);
		// float camX = static_cast<float>(10.0f);
		// float camZ = static_cast<float>(1.0f);
		glm::vec3 viewPosition = glm::vec3(camX, 0.0f, camZ);
		// lookAt函数的参数：1.视角世界位置；2.视角目标位置；3.世界坐标系的上方向
        view = glm::lookAt(viewPosition, glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		int viewLoc = glGetUniformLocation(shaderProgram, "view");
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		int viewPositionLoc = glGetUniformLocation(shaderProgram, "viewPosition");
		glUniform3fv(viewPositionLoc, 1, glm::value_ptr(viewPosition));

		// 3.projection矩阵
		int projectionLoc = glGetUniformLocation(shaderProgram, "projection"); 
		glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));


		// // 正方体本身随着时间不断旋转
		// glm::mat4 model;
		// model = glm::rotate(model, (float)glfwGetTime() * glm::radians(50.0f), glm::vec3(0.5f, 1.0f, 0.0f));
		// glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

		// 4.给所有正方体和地板传入光源的位置
		int lightPositionLoc = glGetUniformLocation(shaderProgram, "lightPosition");
		glUniform3fv(lightPositionLoc, 1, glm::value_ptr(lightPos));


		// ----------渲染正方体-----------
		// 正方体的颜色objectColor
		GLint vertexColorLoaction = glGetUniformLocation(shaderProgram, "objectColor");//获取着色器中uniform变量ourColor的位置
		// glUniform3f(vertexColorLoaction, redValue, greenValue, blueValue);//设置这个objectColor uniform的值为变化色
		glUniform3f(vertexColorLoaction, 1.0f, 1.0f, 1.0f);//TODO:设置objectColor uniform的值为珊瑚色

		// 绑定纹理，自动把纹理赋给片段着色器的采样器
		glActiveTexture(GL_TEXTURE0); // 在绑定纹理之前先激活纹理单元
		glBindTexture(GL_TEXTURE_2D, texture);
		// 渲染三角形，渲染之前，要再次绑定这个节点数组
		glBindVertexArray(VAO); 
		// 渲染10个正方体
		for(unsigned int i = 0; i < 10; i++)
		{
			// 各个正方体先创建model矩阵
			glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);	// 平移
			float angle = 20.0f * i;
			model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));	// 旋转
			int modelLoc = glGetUniformLocation(shaderProgram, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

			// 画一个正方体
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}
		// -------渲染地板-------
		// 画地板，直接使用画正常正方体的shader
		vertexColorLoaction = glGetUniformLocation(shaderProgram, "objectColor");//获取着色器中uniform变量ourColor的位置
		// glUniform3f(vertexColorLoaction, redValue, greenValue, blueValue);//设置这个objectColor uniform的值为变化色
		// 地板自己的光亮度
		glUniform3f(vertexColorLoaction, 1.0f, 1.0f, 1.0f);	//	
		// 绑定地板纹理
		glActiveTexture(GL_TEXTURE0); // 在绑定纹理之前先激活纹理单元
		glBindTexture(GL_TEXTURE_2D, texture_floor);
		model = glm::mat4(1.0f);	// 画地板的时候也要注意，这个地方需要把模型变换矩阵给保持不变
		// view和projection都需要保持不变，因为这是在camera的视角下的！
		int modelLoc_floor_ = glGetUniformLocation(shaderProgram, "model");
		glUniformMatrix4fv(modelLoc_floor_, 1, GL_FALSE, glm::value_ptr(model));
		glBindVertexArray(planeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);




		// --------------------------画光源--------------------------------------------------
		// 激活光源的着色器程序
		glUseProgram(lightShaderProgram);
		// 传入camera的projection矩阵到顶点着色器
		int projectionLoc_ = glGetUniformLocation(lightShaderProgram, "projection"); 
		glUniformMatrix4fv(projectionLoc_, 1, GL_FALSE, glm::value_ptr(projection));
		// 传入camera的view矩阵到顶点着色器
		int viewLoc_ = glGetUniformLocation(lightShaderProgram, "view");
		glUniformMatrix4fv(viewLoc_, 1, GL_FALSE, glm::value_ptr(view));
		// 重新变换光源位置，传入model矩阵到顶点着色器
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, lightPos);	// 移到预先定义好的光源在世界坐标系中的位置
        model = glm::scale(model, glm::vec3(0.2f)); // 缩小这个光源，使得更真实
		int modelLoc_ = glGetUniformLocation(lightShaderProgram, "model");
		glUniformMatrix4fv(modelLoc_, 1, GL_FALSE, glm::value_ptr(model));
		// 绑定并绘制点
		glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);


		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window); 
		glfwPollEvents();
	}
	
	// optional: de-allocate all resources once they've outlived their purpose:
	//   ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &VAO); 
	glDeleteBuffers(1, &VBO); 
	glDeleteProgram(shaderProgram);

	// glfw: terminate, clearing all previously allocated GLFW resources.
	//   ------------------------------------------------------------------
	glfwTerminate(); 
	return 0;
}





// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) 
		glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
// 主要是传入一个窗口，然后调整其宽度和高度。这有利于视口变换。
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and
	// height will be significantly larger than specified on retina displays. 
	glViewport(0, 0, width, height);
	// 0,0表示控制窗口的左下角的坐标是0,0
}