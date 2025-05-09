#include <iostream>
#include <string>
#include <assert.h>
#include <vector>

using namespace std;


// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;

// STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

enum direction {UP, DOWN, LEFT, RIGHT};

struct Sprite 
{
	GLuint VAO;
	GLuint texID;
	vec3 pos;
	vec3 prevpos;
	vec3 dimensions;
    direction moving;
    direction prevmoving;
	float angle;
	float vel;

    vec3 getPMax() {
        double xmax = pos.x + dimensions.x/2;
        double ymax = pos.y + dimensions.y/2;
        return vec3(xmax, ymax, 0);
    }
    vec3 getPMin() {
        double xmin = pos.x - dimensions.x/2;
        double ymin = pos.y - dimensions.y/2;
        return vec3(xmin, ymin, 0);
    }
};

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

int setupShader();
int setupSprite();
int loadTexture(string filePath);
bool checkCollision(Sprite &one, Sprite &two);
void drawSprite(GLuint shaderID, Sprite spr);
void generateFruit(Sprite &food);
void generateBody(vec3 prevPos, std::vector<Sprite>& snakeBody);

const GLuint WIDTH = 816, HEIGHT = 600;

const GLchar *vertexShaderSource = R"(
 #version 400
 layout (location = 0) in vec2 position;
 layout (location = 1) in vec2 texc;
 
 uniform mat4 projection;
 uniform mat4 model;
 out vec2 tex_coord;
 void main()
 {
	tex_coord = vec2(texc.s,1.0-texc.t);
	gl_Position = projection * model * vec4(position, 0.0, 1.0);
 }
 )";

const GLchar *fragmentShaderSource = R"(
 #version 400
in vec2 tex_coord;
out vec4 color;
uniform sampler2D tex_buff;
uniform vec2 offset_tex;
void main()
{
	 color = texture(tex_buff,tex_coord + offset_tex);
}
)";

bool keys[1024];
float FPS = 12.0;
float lastTime = 0.0;

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_SAMPLES, 8);

	for(int i=0; i<1024;i++) { keys[i] = false; }

	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Snake! -- Ender", nullptr, nullptr);
	if (!window)
	{
		std::cerr << "Falha ao criar a janela GLFW" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cerr << "Falha ao inicializar GLAD" << std::endl;
		return -1;
	}

	const GLubyte *renderer = glGetString(GL_RENDERER); /* get renderer string */
	const GLubyte *version = glGetString(GL_VERSION);	/* version as a string */
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	GLuint shaderID = setupShader();

	Sprite background, snakeHead, food;
    std::vector<Sprite> snakeBody;

	GLuint VAO = setupSprite();

	background.VAO = VAO;
	background.texID = loadTexture("../assets/tex/background.png");
	background.pos = vec3(406,308,0);
	background.dimensions = vec3(812 , 616, 1);
	background.angle = 0.0;

	snakeHead.VAO = VAO;
	snakeHead.texID = loadTexture("../assets/sprites/snakeHead.png");
	snakeHead.pos = vec3(28*14+14,28*11+14,0);
	snakeHead.dimensions = vec3(28-1, 28-1, 1);
	snakeHead.vel = 28;
	snakeHead.angle = 0.0;
    snakeHead.moving = RIGHT;
    
	food.VAO = VAO;
	food.texID = loadTexture("../assets/sprites/food.png");
	food.pos = vec3(28*20+14,28*11+14,0);
	food.dimensions = vec3(28-1, 28-1, 1);
	food.vel = 1.5;
	food.angle = 0.0;

	glUseProgram(shaderID); // Reseta o estado do shader para evitar problemas futuros

	double prev_s = glfwGetTime();	// Define o "tempo anterior" inicial.
	double title_countdown_s = 0.1; // Intervalo para atualizar o título da janela com o FPS.

	float colorValue = 0.0;

	glActiveTexture(GL_TEXTURE0);

	glUniform1i(glGetUniformLocation(shaderID, "tex_buff"), 0);

	// Criação da matriz de projeção paralela ortográfica
	mat4 projection = mat4(1); // matriz identidade
	projection = ortho(0.0, 812.0, 0.0, 616.0, -1.0, 1.0);
	// Envio para o shader
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

	//Habilitando transparência/função de mistura
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//Habilitando teste de profundidade
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_ALWAYS);

    //Seed rand
    srand(static_cast<unsigned int>(time(NULL))); 

	// Loop da aplicação - "game loop"
    double accumulator = 0.0;

	while (!glfwWindowShouldClose(window))
	{
		// Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as funções de callback correspondentes
		glfwPollEvents();

		// Limpa o buffer de cor
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // cor de fundo
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        double current_s = glfwGetTime();
        
        // Detect keypresses
		if (keys[GLFW_KEY_LEFT] == true || keys[GLFW_KEY_A] == true)
		{
            if (snakeHead.prevmoving != RIGHT) {
                snakeHead.moving = LEFT;		
            }
		}
		if (keys[GLFW_KEY_RIGHT] == true || keys[GLFW_KEY_D] == true)
		{
            if (snakeHead.prevmoving != LEFT) {
                snakeHead.moving = RIGHT;	
            }
		}
		if (keys[GLFW_KEY_UP] == true || keys[GLFW_KEY_W] == true)
		{
            if (snakeHead.prevmoving != DOWN) {
                snakeHead.moving = UP;
            }
		}
		if (keys[GLFW_KEY_DOWN] == true || keys[GLFW_KEY_S] == true)
		{
            if (snakeHead.prevmoving != UP) {
                snakeHead.moving = DOWN;	
            }
		}
		glUniform2f(glGetUniformLocation(shaderID, "offset_tex"),0.0,0.0);

        // Save snake position
        snakeHead.prevpos = snakeHead.pos;

        // Move snake after "refresh speed"
        double deltaTime = current_s - prev_s;
        prev_s = current_s;
        accumulator += deltaTime;
        const double refreshSpeed = 3 / FPS;
        while (accumulator >= refreshSpeed) {
        // Update snake position based on direction
                switch (snakeHead.moving) {
                case LEFT:
                    snakeHead.pos.x -= snakeHead.vel;
                    break;
                case RIGHT:
                    snakeHead.pos.x += snakeHead.vel;
                    break;
                case UP:
                    snakeHead.pos.y += snakeHead.vel;
                    break;
                case DOWN:
                    snakeHead.pos.y -= snakeHead.vel;
                    break;
            }
            snakeHead.prevmoving = snakeHead.moving;
            accumulator -= refreshSpeed;

            for (int i=0; i < snakeBody.size(); i++)
            {
                snakeBody[i].prevpos = snakeBody[i].pos;
                if (i == 0) {
                    snakeBody[i].pos = snakeHead.prevpos;
                }
                else 
                {
                    snakeBody[i].pos = snakeBody[i-1].prevpos;
                }
            }

            if (checkCollision(snakeHead, food)) {
                generateFruit(food);
                if (snakeBody.size() > 0) {
                    generateBody(snakeBody[(snakeBody.size())-1].prevpos, snakeBody);
                }
                else {
                    generateBody(snakeHead.prevpos, snakeBody);
                }
            }
        }
        // Snake out of bounds
        if (snakeHead.pos.x < 0 || snakeHead.pos.x > 812 || snakeHead.pos.y < 0 || snakeHead.pos.y > 616)
        {
		    glfwSetWindowShouldClose(window, GL_TRUE);
        }
        // Snake self collide
        for (int i=0; i < snakeBody.size(); i++){
            if (checkCollision(snakeHead, snakeBody[i]))
            {
    		    glfwSetWindowShouldClose(window, GL_TRUE);
            }
        }

		drawSprite(shaderID,background);
		drawSprite(shaderID,snakeHead);
		drawSprite(shaderID,food);
        for (int i=0; i < snakeBody.size(); i++){
            drawSprite(shaderID,snakeBody[i]);
        }
		glfwSwapBuffers(window);
	}
	// Finaliza a execução da GLFW, limpando os recursos alocados por ela
	glfwTerminate();
	return 0;
}

bool checkCollision(Sprite &one, Sprite &two)
{
    // collision x-axis?
    bool collisionX = one.getPMax().x >=
    two.getPMin().x &&
    two.getPMax().x >= one.getPMin().x;
    // collision y-axis?
    bool collisionY = one.getPMax().y >=
    two.getPMin().y &&
    two.getPMax().y >= one.getPMin().y;
    // collision only if on both axes
    return collisionX && collisionY;
}

void generateFruit(Sprite &food) 
{
    int cols = 29; //W28
    int rows = 22; //H21

    int col = rand() % cols;
    int row = rand() % rows;

    food.pos.x = 14 + col * 28;
    food.pos.y = 14 + row * 28;
}

void generateBody(vec3 prevHead, std::vector<Sprite>& snakeBody) {
	GLuint VAO = setupSprite();
    Sprite newBody;
	newBody.VAO = VAO;
	newBody.texID = loadTexture("../assets/sprites/snakeBody.png");
	newBody.pos = prevHead;
	newBody.dimensions = vec3(7 * 4-1, 7 * 4-1, 1);
    newBody.angle = 0;
    snakeBody.push_back(newBody);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (action == GLFW_PRESS)
	{
		keys[key] = true;
	}
	else if (action == GLFW_RELEASE)
	{
		keys[key] = false;
	}
}

int setupShader()
{
	// Vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	// Checando erros de compilação (exibição via log no terminal)
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}
	// Fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	// Checando erros de compilação (exibição via log no terminal)
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}
	// Linkando os shaders e criando o identificador do programa de shader
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// Checando por erros de linkagem
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
				  << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

int setupSprite()
{
	// Aqui setamos as coordenadas x, y e z do triângulo e as armazenamos de forma
	// sequencial, já visando mandar para o VBO (Vertex Buffer Objects)
	// Cada atributo do vértice (coordenada, cores, coordenadas de textura, normal, etc)
	// Pode ser arazenado em um VBO único ou em VBOs separados
	GLfloat vertices[] = {
		// x   y   s     t
		-0.5,
		0.5,
		0.0,
		1.0,
		-0.5,
		-0.5,
		0.0,
		0.0,
		0.5,
		0.5,
		1.0,
		1.0,

		-0.5,
		-0.5,
		0.0,
		0.0,
		0.5,
		-0.5,
		1.0,
		0.0,
		0.5,
		0.5,
		1.0,
		1.0,
	};

	GLuint VBO, VAO;
	// Geração do identificador do VBO
	glGenBuffers(1, &VBO);
	// Faz a conexão (vincula) do buffer como um buffer de array
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// Envia os dados do array de floats para o buffer da OpenGl
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Geração do identificador do VAO (Vertex Array Object)
	glGenVertexArrays(1, &VAO);
	// Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
	// e os ponteiros para os atributos
	glBindVertexArray(VAO);
	// Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para o atributo), indicando:
	//  Localização no shader * (a localização dos atributos devem ser correspondentes no layout especificado no vertex shader)
	//  Numero de valores que o atributo tem (por ex, 3 coordenadas xyz)
	//  Tipo do dado
	//  Se está normalizado (entre zero e um)
	//  Tamanho em bytes
	//  Deslocamento a partir do byte zero

	// Ponteiro pro atributo 0 - Posição - coordenadas x, y
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);

	// Ponteiro pro atributo 2 - Coordenada de textura - coordenadas s,t
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid *)(2 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice
	// atualmente vinculado - para que depois possamos desvincular com segurança
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
	glBindVertexArray(0);

	return VAO;
}

void drawSprite(GLuint shaderID, Sprite spr)
{
	// Neste código, usamos o mesmo buffer de geomtria para todos os sprites
	glBindVertexArray(spr.VAO);				 // Conectando ao buffer de geometria
		
	// Desenhar o sprite 1
	glBindTexture(GL_TEXTURE_2D, spr.texID); // Conectando ao buffer de textura
	// Criação da  matriz de transformações do objeto
	mat4 model = mat4(1);  // matriz identidade
	model = translate(model, spr.pos);
	model = rotate(model, radians(spr.angle),vec3(0.0,0.0,1.0));
	model = scale(model, spr.dimensions);
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));
	// Chamada de desenho - drawcall
	// Poligono Preenchido - GL_TRIANGLES
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindVertexArray(0); // Desnecessário aqui, pois não há múltiplos VAOs
}

int loadTexture(string filePath)
{
	GLuint texID;

	// Gera o identificador da textura na memória
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	int width, height, nrChannels;

	unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

	if (data)
	{
		if (nrChannels == 3) // jpg, bmp
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		else // png
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}

	stbi_image_free(data);

	glBindTexture(GL_TEXTURE_2D, 0);

	return texID;
}