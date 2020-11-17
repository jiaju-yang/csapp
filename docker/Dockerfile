FROM debian:10
 
ARG SSH_USER
ARG SSH_PASS

RUN sed -i 's/deb.debian.org/mirrors.aliyun.com/g' /etc/apt/sources.list
RUN sed -i 's/security.debian.org/mirrors.aliyun.com/g' /etc/apt/sources.list

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
    openssh-server \
    g++ \
    gcc-multilib \
    make \
    gdb \
    zsh \
    git \
    ca-certificates \
    wget \
    vim
 
COPY ssh_config /etc/ssh/ssh_config
COPY sshd_config /etc/ssh/sshd_config
 
COPY user.sh /usr/local/bin/user.sh
RUN chmod +x /usr/local/bin/user.sh
RUN /usr/local/bin/user.sh

COPY --chown=${SSH_USER}:ssh oh_my_zsh_install.sh /home/${SSH_USER}/.oh_my_zsh_install.sh
RUN chmod +x /home/${SSH_USER}/.oh_my_zsh_install.sh
USER "${SSH_USER}"
RUN ~/.oh_my_zsh_install.sh
RUN git clone https://github.com/zsh-users/zsh-autosuggestions.git ~/.oh-my-zsh/custom/plugins/zsh-autosuggestions
RUN git clone https://github.com/zsh-users/zsh-syntax-highlighting.git ~/.oh-my-zsh/custom/plugins/zsh-syntax-highlighting

USER root
COPY --chown=${SSH_USER}:ssh zshrc /home/${SSH_USER}/.zshrc
COPY --chown=${SSH_USER}:ssh lambda-mod.zsh-theme /home/${SSH_USER}/.oh-my-zsh/custom/themes/lambda-mod.zsh-theme

COPY entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod +x /usr/local/bin/entrypoint.sh
 
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]

CMD tail -f /dev/null
