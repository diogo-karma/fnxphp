PHP_ARG_ENABLE(fnx, whether to enable fnx support,
[  --enable-fnx           Enable fnx support])

AC_ARG_ENABLE(fnx-debug,
[  --enable-fnx-debug     Enable fnx debug mode default=no],
[PHP_FNX_DEBUG=$enableval],
[PHP_FNX_DEBUG="no"])  

if test "$PHP_FNX" != "no"; then

  if test "$PHP_FNX_DEBUG" = "yes"; then
    AC_DEFINE(PHP_FNX_DEBUG,1,[define to 1 if you want to change the POST/GET by php script])
  else
    AC_DEFINE(PHP_FNX_DEBUG,0,[define to 1 if you want to change the POST/GET by php script])
  fi

  AC_MSG_CHECKING([PHP version])

  tmp_version=$PHP_VERSION
  if test -z "$tmp_version"; then
    if test -z "$PHP_CONFIG"; then
      AC_MSG_ERROR([php-config not found])
    fi
    php_version=`$PHP_CONFIG --version 2>/dev/null|head -n 1|sed -e 's#\([0-9]\.[0-9]*\.[0-9]*\)\(.*\)#\1#'`
  else
    php_version=`echo "$tmp_version"|sed -e 's#\([0-9]\.[0-9]*\.[0-9]*\)\(.*\)#\1#'`
  fi

  if test -z "$php_version"; then
    AC_MSG_ERROR([failed to detect PHP version, please report])
  fi

  ac_IFS=$IFS
  IFS="."
  set $php_version
  IFS=$ac_IFS
  fnx_php_version=`expr [$]1 \* 1000000 + [$]2 \* 1000 + [$]3`

  if test "$fnx_php_version" -le "5002000"; then
    AC_MSG_ERROR([You need at least PHP 5.2.0 to be able to use this version of Fnx. PHP $php_version found])
  else
    AC_MSG_RESULT([$php_version, ok])
  fi
  PHP_NEW_EXTENSION(fnx, fnx.c fnx_application.c fnx_bootstrap.c fnx_dispatcher.c fnx_exception.c fnx_config.c fnx_request.c fnx_response.c fnx_view.c fnx_controller.c fnx_action.c fnx_router.c fnx_loader.c fnx_registry.c fnx_plugin.c fnx_session.c, $ext_shared)
fi
