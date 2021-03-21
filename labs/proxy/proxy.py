import click
import socket

MAX_LINE_LENGTH = 8192


def parse_request(conn):
    method, uri, version = conn.recv(
        MAX_LINE_LENGTH).decode('utf-8').split()
    print(f'method: {method}; uri: {uri}; version: {version}')
    headers = {}
    while True:
        line = conn.recv(MAX_LINE_LENGTH)
        if line == b'\r\n':
            break
        print(line.decode('utf-8'))
        k, v = line.decode('utf-8').split(':', 1)
        headers[k.strip()] = v.strip()
    return method, uri, version, headers


def forward(host, method, uri, version, headers):
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect((host, 80))
    request = '{}\n{}\n\r\n'.format(' '.join([method, uri, version]), '\n'.join([
                                    f'{k}: {v}' for k, v in headers.items()]))
    client_socket.send(request.encode('utf-8'))
    recv_data = client_socket.recv(MAX_LINE_LENGTH)
    client_socket.close()
    return recv_data


def run_server(port):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        server_socket.bind((socket.gethostname(), port))
    except OSError:
        print(f'port already in use: {port}')
        return
    server_socket.listen(5)
    print(f'Server listening at: {port}')
    while True:
        conn, (ip, port) = server_socket.accept()
        print(f'Accepted connection from ({ip}, {port})')
        try:
            method, uri, version, headers = parse_request(conn)
        except ValueError:
            print('Invalid request.')
            conn.send('Invalid request.\n'.encode('utf-8'))
            conn.close()
            continue
        host = headers.get('Host', headers.get('host', None))
        if not host:
            print('No host. Cannot forward.')
            conn.close()
            continue
        if method != 'GET':
            print('Not implemented method.')
            conn.close()
            continue
        data = forward(host, method, uri, version, headers)
        conn.sendall(data)
        conn.close()
    server_socket.close()


@click.command()
@click.argument('port', type=click.INT)
def main(port):
    run_server(port)


if __name__ == '__main__':
    main()
