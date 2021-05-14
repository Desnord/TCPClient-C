#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "listas.h"
#include "clientfuncs.h"

/* SOCKET ------------------------- */
#include <netdb.h>
#include <sys/unistd.h>
#include <arpa/inet.h>
typedef struct sockaddr SockAddr;
typedef struct sockaddr_in SockAddr_in;

#define CONFIG "../ClientFiles/config.txt" // arquivo de configuracao

/* -------------------------- funcoes de recebimento e envio de informacoes -------------------------- */

/* funcao para receber um perfil do servidor */
Perfil *recebePerfil(int socketFD)
{
    // para guardar informacoes do perfil
    char *values[5];
    int ano;
    NoString *auxexp, *auxhab;

    // recebe tamanhos das informacoes basicas do perfil
    char str[12];
    int tams[5];
    for(int i=0; i<5; i++)
    {
        // recebe tamanho
        read(socketFD, str, 12);
        tams[i] = (int)strtol(str,NULL,10);
    }

    // aloca memoria para as informacoes
    for(int i=0; i<5; i++)
        values[i] = malloc(sizeof(char)*(tams[i]+1));

    // recebe dados basicos do perfil
    for(int i=0; i<5; i++)
        read(socketFD, values[i], tams[i]+1);
    read(socketFD, str, 12);
    ano = (int)strtol(str,NULL,10);

    // aloca lista dos dados mais complexos
    auxhab = newStringList();
    auxexp = newStringList();

    // recebe dados mais complexos
    for(int j=0; j<2; j++)
    {
        // recebe a qtde de habilidades/experiencias que serao lidas
        read(socketFD, str, 12);
        int qtd = (int)strtol(str,NULL,10);

        NoString *auxEH;
        auxEH = !j ? auxhab : auxexp;

        // recebe habilidades/experiencias
        for(int i=0; i<qtd; i++)
        {
            // recebe tamanho da habilidade/experiencia
            read(socketFD, str, 12);
            int tam = (int)strtol(str,NULL,10);

            // aloca memoria para a habilidade/experiencia
            char *habexp = malloc(sizeof(char)*(tam+1));

            // recebe habilidade/experiencia e armazena na lista
            read(socketFD,habexp,tam+1);
            stringListInsert(auxEH,habexp);
        }
    }
    // alloca perfil, atribui valores e o retorna (sucesso no recebimento)
    return createPerfil(values[0],values[1],values[2],ano,values[3],values[4],auxexp,auxhab);
}

/* funcao que envia um perfil através do socket */
void enviaPerfil(int socketFD, Perfil *p)
{
    // envia tamanho das informacoes basicas
    char str[12];
    int tams[5] = {(int)strlen(p->email),(int)strlen(p->nome),(int)strlen(p->sobrenome),(int)strlen(p->formacao),(int)strlen(p->cidade_residencia)};
    for(int i=0; i<5; i++)
    {
        sprintf(str, "%d", tams[i]);
        write(socketFD, str, 12);
    }

    // envia informacoes basicas
    write(socketFD, p->email, tams[0]+1);
    write(socketFD, p->nome, tams[1]+1);
    write(socketFD, p->sobrenome, tams[2]+1);
    write(socketFD, p->formacao, tams[3]+1);
    write(socketFD, p->cidade_residencia, tams[4]+1);

    sprintf(str, "%d", p->ano_formatura);
    write(socketFD, str, 12);

    // percorre as listas de experiencia e habilidade, enviando as informacoes
    int tamLS = stringListLen(p->habilidades);
    for(int i=0; i<2; i++)
    {
        // envia tamanho da lista (hab ou exp)
        sprintf(str, "%d", tamLS);
        write(socketFD, str, 12);

        if (tamLS > 0)
        {
            // escolhe qual lista esta sendo percorrida (hab ou exp)
            NoString *at;
            at = !i ?  p->habilidades->prox : p->experiencia->prox;

            while(at != NULL)
            {
                // envia tamanho da habilidade/experiencia atual
                sprintf(str, "%d", (int)strlen(at->str));
                write(socketFD, str, 12);

                // envia habilidade/experincia
                write(socketFD,at->str,(int)strlen(at->str)+1);

                //passa para a proxima hab/exp
                at = at->prox;
            }
        }
        tamLS = stringListLen(p->experiencia);
    }
}

/* funcao para receber todos os perfis do servidor */
NoPerfil  *recebeTodos(int socketFD)
{
    // recebe quantidade de perfis
    char str[12];
    read(socketFD, str, 12);

    // inicializa lista, alocando memoria
    NoPerfil *listaGeral = newPerfilList();

    // recebe todos os perfis e vai adicionando na lista
    for(int i=(int)strtol(str,NULL,10); i>0; i--)
    {
        Perfil *p = recebePerfil(socketFD);
        perfilListInsert(listaGeral,p);
    }
    return listaGeral;
}

/* funcoes para receber todos os perfis reduzidos (de acordo com a selecao escolhida) */
NoPerfilEmailNomeCurso *NPENCrecebeTodos(int socketFD, int tam)
{
    // cria lista para retorno
    NoPerfilEmailNomeCurso *listaRet = newNPENCList();

    // recebe todos os perfis reduzidos
    for(int i=0; i<tam; i++)
    {
        // aloca perfil
        PerfilEmailNomeCurso *penc = (PerfilEmailNomeCurso*)malloc(sizeof(PerfilEmailNomeCurso));

        // recebe tamanhos das informacoes
        char str[12];
        int tams[4];

        for(int j=0; j<4; j++)
        {
            read(socketFD, str, 12);
            tams[j] = (int)strtol(str,NULL,10);
        }

        char *infos[4];

        // aloca memoria para as variaveis
        for(int j=0; j<4; j++)
            infos[j] = malloc(sizeof(char)*(tams[j]+1));

        // recebe informações
        for(int j=0; j<4; j++)
            read(socketFD,infos[j],(tams[j]+1));

        // atribui valores a variavel de perfil auxiliar
        penc->email = infos[0];
        penc->nome = infos[1];
        penc->sobrenome = infos[2];
        penc->formacao = infos[3];

        NPENCListInsert(listaRet,penc); // insere estrutura na lista de retorno
    }
    return listaRet; // retorna lista
}

NoPerfilEmailNome *NPENrecebeTodos(int socketFD, int tam)
{
    // cria lista para retorno
    NoPerfilEmailNome *listaRet = newNPENList();

    for(int i=0; i<tam; i++)
    {
        // aloca perfil auxiliar
        PerfilEmailNome *penc = (PerfilEmailNome*)malloc(sizeof(PerfilEmailNome));

        // recebe tamanhos das informacoes
        char str[12];
        int tams[3];

        for(int j=0; j<3; j++)
        {
            read(socketFD, str, 12);
            tams[j] = (int)strtol(str,NULL,10);
        }

        // aloca memoria para as informacoes
        char *infos[3];
        for(int j=0; j<3; j++)
            infos[j] = malloc(sizeof(char)*(tams[j]+1));

        // recebe informações
        for(int j=0; j<3; j++)
            read(socketFD,infos[j],(tams[j]+1));

        // atribui valores a variavel de perfil auxiliar
        penc->email = infos[0];
        penc->nome = infos[1];
        penc->sobrenome = infos[2];


        NPENListInsert(listaRet,penc); // insere estrutura na lista de retorno
    }
    return listaRet; // retorna lista
}

/* funcao de comunicação entre cliente-servidor [gerencia as 8 operações] */
void comunicacao(int socketFD, char *perm)
{
    char opt[20];
	
    for (;;)
    {
    	memset(opt,'\0',20);
        printMenu(1);
        printf("Escolha uma opção: ");
        gets(opt);
        printf("\n");

        write(socketFD, opt, 20); // envia opção escolhida ao servidor.

        if (opt[0] == '0')
            break;
        
        if (opt[0] == '1' || opt[0] == '2')
        {
            // le curso ou habilidade
            char buffer[200];
			memset(buffer,'\0',200);
			
			if(opt[0] == '1')
				printf("Digite o curso: ");
			else
				printf("digite a habilidade: ");
	
			gets(buffer);
			write(socketFD, buffer, 200); // envia habilidade ou curso ao servidor
			
			// recebe tamanho da lista
			char str[12];
			read(socketFD, str, 12);

            // recebe lista
            NoPerfilEmailNome *lista = NPENrecebeTodos(socketFD, (int)strtol(str, NULL, 10));

            printListaNomeEmail(lista); // imprime dados
            NPENListFree(lista);        // libera memoria alocada
        }
        else if (opt[0] == '3')
        {
			// le curso ou habilidade
			char buffer[200];
			memset(buffer,'\0',200);
			
			// le ano a ser filtrado
			printf("digite o ano: ");
			gets(buffer);

            // envia ano a ser filtrado
            write(socketFD, buffer, 12);
            
            // recebe tamanho da lista
			char str[12];
            read(socketFD, str, 12);

            // recebe lista
            NoPerfilEmailNomeCurso *lista = NPENCrecebeTodos(socketFD, (int)strtol(str, NULL, 10));

            printListaNomeEmailCurso(lista); // imprime dados
            NPENCListFree(lista);            // libera memoria alocada
        }
        else if (opt[0] == '4')
        {
            NoPerfil *lista = recebeTodos(socketFD); // recebe todos os perfis
            printListaPerfil(lista); 				 // imprime informacoes
            perfilListFree(lista);   				 // libera memoria alocada
        }
        else if (opt[0] == '5')
        {
			// le email para busca
			char buffer[200];
			memset(buffer,'\0',200);
			
			printf("Digite o email: ");
			gets(buffer);
			printf("\n");
			
            // envia email ao servidor
            write(socketFD, buffer, 200);

            // le resposta do servidor, quanto a existencia do perfil
            char res = 'e';
            read(socketFD, &res, 1);
            printResp(res,5);

            if(res == '1')
            {
                Perfil *p = recebePerfil(socketFD); // recebe informacoes do perfil
                printPerfil(p);						// imprime suas informacoes
                freePerfil(p);						// libera memoria alocada
                printf("\n");
            }
        }
        else if(!strcmp(perm,"Admin"))
		{
			if (opt[0] == '6')
			{
				// le informacoes simples do perfil que vai ser adicionado
				char *infos[6], labels[6][23] = {"o email","o nome","o sobrenome","a cidade de residencia","a formacao","o ano de formatura"};
				
				for(int i=0; i<6; i++)
				{
					infos[i] = malloc(sizeof(char)*200);
					printf("Digite %s: ",labels[i]);
					gets(infos[i]);
				}
				
				// aloca memoria para os dados mais complexos
				NoString *auxexp = newStringList();
				NoString *auxhab = newStringList();
				
				// Cria perfil, atribuindo dados e ponteiros
				Perfil *p = createPerfil(infos[0],infos[1],infos[2],(int)strtol(infos[5],NULL,10),infos[3],infos[4],auxexp,auxhab);
				
				// leitura dos dados mais complexos
				char opt2[20];
				printf("Inserir habilidade (s) ou (n): ");
				gets(opt2);
				
				while (!strcmp(opt2,"s"))
				{
					printf("Digite uma habilidade: ");
					char *exphab = malloc(sizeof(char)*100);
					gets(exphab);
					!existeString(exphab,auxhab) ? stringListInsert(auxhab,exphab) : free(exphab);
					
					printf("Continuar inserindo habilidades (s) ou (n): ");
					gets(opt2);
				}
				
				printf("Inserir experiência (s) ou (n): ");
				gets(opt2);
				while (!strcmp(opt2,"s"))
				{
					printf("Digite uma experiência: ");
					char *exphab = malloc(sizeof(char)*100);
					gets(exphab);
					!existeString(exphab,auxhab) ? stringListInsert(auxhab,exphab) : free(exphab);
					
					printf("Continuar inserindo experiência (s) ou (n): ");
					gets(opt2);
				}
				
				enviaPerfil(socketFD, p); 		// envia perfil
				freePerfil(p);			  		// libera memoria utilizada
				char res = '0';			  		// recebe flag do resultado da insercao
				read(socketFD, &res, 1);	//
				printResp(res,6);			//
			}
			else if(opt[0] == '7' || opt[0] == '8')
			{
				// le email do perfil que vai ser atualizado e a experiência à ser adicionada
				char email[200], exp[200];
				memset(email,'\0',200);
				memset(exp,'\0',200);
				printf("Digite o email: ");
				gets(email);
				
				if(opt[0] == '7')
				{
					printf("Digite a experiencia: ");
					gets(exp);
					
					// envia email e experiência ao servidor
					write(socketFD, email, 200);
					write(socketFD, exp, 200);
				}
				else if(opt[0] == '8')
				{
					// envia email ao servidor
					write(socketFD, email, 200);
				}
				
				// recebe resposta da operação
				char res = '0';
				read(socketFD, &res, 1);
				printResp(res,opt[0]);
			}
			else
			{
				PRINTCL(CLLR,"Opção inválida.\n");
			}
		}
        else
		{
        	PRINTCL(CLLR,"Opção inválida.\n");
		}
	
		printf("Pressione uma tecla para continuar...");
        getchar();
        printf("\n");
        system("clear");
    }
}

int getConfig(char *ip, char *user) // le ip, porta e tipo de usuario do arquivo (retorna porta)
{
	// config [le ip, porta e usuario do arquivo]
	FILE *config = fopen(CONFIG,"r");
	int port = 9000;
	char str[12];
	
	if(config != NULL)
	{
		fscanf(config,"%s",user);
		fscanf(config,"%s",ip);
		fscanf(config,"%s",str);
		port = (int)strtol(str,NULL,10);
		fclose(config);
	}
	return port;
}
void printConnStatus(char t, char *ip) // trata encerramento do programa em erro de conexão
{
	if(t == 'E')
	{
		PRINT2C(" erro de conexão.\n",CLLR,CLW," pressione uma tecla para encerrar...");
		getchar();
		exit(0);
	}
	else
	{
		PRINT2C("conectado ao servidor: ",CLLG,CLY,"%s\n",ip);
	}
}
/* ----------------------------------------------------- MAIN ------------------------------------------------------ */
int main()
{
	// le ip, porta e tipo de usuario no arquivo de configuracao
	char user[7] = "Normal";        // valor (default)
	char ip[16] = "127.0.0.1";      // valor (default)
	int port = getConfig(ip,user);  // valor default é 9000
	
	SockAddr_in server;                             					// estrutura do socket
	int socketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // cria file descriptor do socket TCP
	
	if(socketFD == -1)
		printConnStatus('E', ip);
	
	// atribui valores à estrutura do socket
	server.sin_family = AF_INET;            // AF_INET é a familia de protocolos do IPV4
	server.sin_addr.s_addr = inet_addr(ip); // ip da máquina executando o servidor
	server.sin_port = htons(port);          // porta do servidor
	
	int conn = connect(socketFD, (SockAddr*)&server, sizeof(server));
	
	if(conn == -1)
		printConnStatus('E', ip);
	
	printConnStatus('S', ip);
	comunicacao(socketFD, user); // realiza troca de mensagem com o servidor
	close(socketFD);             // encerra socket
	return 0;
}