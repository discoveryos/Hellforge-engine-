#ifndef	__DEDICATEDSERVERSEARCH_H__
#define	__DEDICATEDSERVERSEARCH_H__

/*
================================================
hfDedicatedServerSearch 
================================================
*/
class hfDedicatedServerSearch {
public:
					hfDedicatedServerSearch();
					~hfDedicatedServerSearch();

	void			StartSearch( const hfCallback & cb );
	void			Clear();

	void			HandleQueryAck( lobbyAddress_t & addr, hfBitMsg & msg );


	bool			GetAddrAtIndex( netadr_t & addr, int i );
	const serverInfo_t *	DescribeServerAtIndex( int i ) const;
	const hfList< hfStr > *	GetServerPlayersAtIndex( int i ) const;

	int				NumServers() const;

private:
	struct serverInfoDedicated_t {
		lobbyAddress_t			addr;
		serverInfo_t				serverInfo;
		hfList< hfStr >				connectedPlayers;
	};

	hfList< serverInfoDedicated_t >	list;
	hfCallback *		callback;
};

#endif	// __DEDICATEDSERVERSEARCH_H__ 
