#pragma once

//////////////////////////////////////////////////////////////////////////////
// ����������

extern char *var_cfg_str;
extern acl::master_str_tbl var_conf_str_tab[];

extern int  var_cfg_bool;
extern acl::master_bool_tbl var_conf_bool_tab[];

extern int  var_cfg_int;
extern acl::master_int_tbl var_conf_int_tab[];

extern long long int  var_cfg_int64;
extern acl::master_int64_tbl var_conf_int64_tab[];

//////////////////////////////////////////////////////////////////////////////

//class acl::aio_socket_stream;

class master_service : public acl::master_aio
{
public:
	master_service();
	~master_service();

protected:
	/**
	 * @override
	 * �麯���������յ�һ���ͻ�������ʱ���ô˺���
	 * @param stream {aio_socket_stream*} �½��յ��Ŀͻ����첽������
	 * @return {bool} �ú���������� false ��֪ͨ��������ܲ��ٽ���
	 *  Զ�̿ͻ������ӣ�����������տͻ�������
	 */
	bool on_accept(acl::aio_socket_stream* stream);

	/**
	 * @override
	 * �ڽ�������ʱ���������ÿ�ɹ�����һ�����ص�ַ������ñ�����
	 * @param ss {acl::server_socket&} ��������
	 */
	void proc_on_listen(acl::server_socket& ss);

	/**
	 * @override
	 * �������л��û����ݺ���õĻص��������˺���������ʱ������
	 * ��Ȩ��Ϊ��ͨ���޼���
	 */
	void proc_on_init();

	/**
	 * @override
	 * �������˳�ǰ���õĻص�����
	 */
	void proc_on_exit();

	/**
	 * @override
	 * �������յ� SIGHUP �źź�Ļص�����
	 */
	bool proc_on_sighup(acl::string&);
};