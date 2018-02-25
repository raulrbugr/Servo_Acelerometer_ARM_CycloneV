
rmmod pushbutton_irq_handler
echo '*********************'
echo '*Modulo desinstalado*'
echo '*********************'

make clean
echo '*********************'
echo '*   Modulo limpio   *'
echo '*********************'

make
echo '*********************'
echo '* Modulo construido *'
echo '*********************'

insmod pushbutton_irq_handler.ko
echo '*********************'
echo '* Modulo instalado  *'
echo '*********************'

