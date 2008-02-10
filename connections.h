

namespace Konnekt {
	namespace Connections {

		//  Obsluga polaczen
		class Connection {
		public:
			//    int plug; // index
			int retry; // liczba prob
			bool connect; // Czy laczyc (powinno byc false jezeli rozlaczyl uzytkownik!) ...
			__time64_t waitTime;
			Connection() {retry = 0;connect = 0;waitTime = 0;}
		};
		typedef map <int , Connection> tList;

		Connection & get(int id);
		tList & getList();

		void check();
		void connect();
		void setConnect(int sender , int connect);
		void startUp();
		bool isConnected();


	};
};