#用于配置Django项目，以支持异步Web服务器，允许服务器处理多个请求而不需要为每个请求创建新的线程或进程，从而提高了性能和资源利用率
"""
ASGI config for dianshan project.

It exposes the ASGI callable as a module-level variable named ``application``.

For more information on this file, see
https://docs.djangoproject.com/en/3.1/howto/deployment/asgi/
"""

import os

from django.core.asgi import get_asgi_application

os.environ.setdefault('DJANGO_SETTINGS_MODULE', 'CarSuv.settings')
#设置环境变量，引用setting文件
application = get_asgi_application()
